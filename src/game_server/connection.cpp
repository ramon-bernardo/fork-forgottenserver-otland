#include "connection.h"

#include "../game.h"
#include "../lockfree.h"
#include "../rsa.h"
#include "../scheduler.h"
#include "../tasks.h"
#include "../xtea.h"
#include "incoming_message.h"

#include <boost/asio.hpp>
#include <boost/asio/dispatch.hpp>
#include <fmt/core.h>
#include <random>

extern Game g_game;
extern Scheduler g_scheduler;

class Connection;

namespace {

std::vector<std::shared_ptr<Connection>> auto_flush_connections;

void auto_flush(const std::vector<std::shared_ptr<Connection>>& connections)
{
	g_scheduler.addEvent(createSchedulerTask(10, [&]() {
		for (const auto& connection : connections) {
			if (auto& msg = connection->getCurrentBuffer()) {
				protocol->send(std::move(msg));
			}
		}

		if (!connections.empty()) {
			auto_flush(connections);
		}
	}));
}

void XTEA_encrypt(OutputMessage& msg, const xtea::round_keys& key)
{
	// The message must be a multiple of 8
	size_t paddingBytes = msg.getLength() % 8u;
	if (paddingBytes != 0) {
		msg.addPaddingBytes(8 - paddingBytes);
	}

	uint8_t* buffer = msg.getOutputBuffer();
	xtea::encrypt(buffer, msg.getLength(), key);
}

bool XTEA_decrypt(IncomingMessage& msg, const xtea::round_keys& key)
{
	if (((msg.getLength() - 6) & 7) != 0) {
		return false;
	}

	uint8_t* buffer = msg.getRemainingBuffer();
	xtea::decrypt(buffer, msg.getLength() - 6, key);

	uint16_t innerLength = msg.get<uint16_t>();
	if (innerLength + 8 > msg.getLength()) {
		return false;
	}

	msg.setLength(innerLength);
	return true;
}

} // namespace

using namespace std::chrono_literals;

namespace tfs::game_server {

Connection::Connection(asio::ip::tcp::socket&& socket) : stream{std::move(socket)} {}

Connection::~Connection() { shutdown(); }

void Connection::run()
{
	// Starts the connection logic by dispatching the initial read operation
	// on the associated executor. This ensures the async chain begins
	// in the correct I/O context thread.
	dispatch(stream.get_executor(), [self = shared_from_this()] { self->read_server_name(); });
}

void Connection::read_server_name()
{
	std::lock_guard<std::recursive_mutex> lock(mtx);

	// First step in a handshake.
	// Start an read for the initial server name msg.

	stream.async_read_some(asio::buffer(msg.body(), 256),
	                       [self = shared_from_this()](beast::error_code ec, size_t bytes_transferred) {
		                       self->on_read_server_name(ec, bytes_transferred);
	                       });
}

void Connection::on_read_server_name(beast::error_code ec, size_t bytes_transferred)
{
	std::lock_guard<std::recursive_mutex> lock(mtx);

	if (ec) {
		fmt::print(stderr, "{}: {}\n", __FUNCTION__, ec.message());
		close_and_shutdown();
		return;
	}

	if (disconnected) {
		return;
	}

	// Update msg header length with the received bytes.
	msg.set_header_len(bytes_transferred);

	// Extract the server name string from the received data.
	const auto server_name = msg.get<std::string>();
	if (server_name.empty()) {
		// Server name is missing or invalid, abort the handshake.
		return;
	}

	// Prepare a challenge to send back to the client for handshake validation.
	static std::random_device rd;
	static std::ranlux24 generator(rd());
	static std::uniform_int_distribution<uint16_t> randNumber(0, 255);
	challenge.timestamp = static_cast<uint32_t>(time(nullptr));
	challenge.random_number = randNumber(generator);
	// Send the generated challenge to the client.
	write_challenge(challenge.timestamp, challenge.random_number);

	// Proceed to the next step: reading the login header.
	read_login_header();
}

void Connection::read_login_header()
{
	std::lock_guard<std::recursive_mutex> lock(mtx);

	if (disconnected) {
		return;
	}

	// Set a short timeout for the next read operation.
	stream.expires_after(5s);

	// Clear the msg buffer before reading the next msg.
	msg.reset();

	// Read the first two bytes, which represent the login packet header.
	async_read(stream, asio::buffer(msg.as_bytes(), 2),
	           [self = shared_from_this()](beast::error_code ec, size_t bytes_transferred) {
		           self->on_read_login_header(ec, bytes_transferred);
	           });
}

void Connection::on_read_login_header(beast::error_code ec, size_t bytes_transferred)
{
	std::lock_guard<std::recursive_mutex> lock(mtx);

	if (ec) {
		fmt::print(stderr, "{}: {}\n", __FUNCTION__, ec.message());
		close_and_shutdown();
		return;
	}

	if (disconnected) {
		return;
	}

	// Disable the timeout now that the header has been received successfully.
	stream.expires_never();

	// Extract the expected length of the login payload.
	auto len = msg.header_len();
	if (len == 0 || len > 1024) {
		// Malformed or suspicious packet length, terminate the process.
		close_and_shutdown();
		return;
	}

	// Resize the msg buffer to fit the incoming payload length.
	msg.truncate(len);

	read_login_body();
}

void Connection::read_login_body()
{
	std::lock_guard<std::recursive_mutex> lock(mtx);

	if (disconnected) {
		return;
	}

	// Apply a short timeout for the login body read.
	stream.expires_after(5s);

	// Read the remainder of the login msg based on the previously received header length.
	async_read(stream, asio::buffer(msg.body(), msg.header_len()),
	           [self = shared_from_this()](beast::error_code ec, size_t bytes_transferred) {
		           self->on_read_login_body(ec, bytes_transferred);
	           });
}

void Connection::on_read_login_body(beast::error_code ec, size_t bytes_transferred)
{
	std::lock_guard<std::recursive_mutex> lock(mtx);

	if (ec) {
		fmt::print(stderr, "{}: {}\n", __FUNCTION__, ec.message());
		close_and_shutdown();
		return;
	}

	if (disconnected) {
		return;
	}

	// Disable timeout since the read completed successfully.
	stream.expires_never();

	// Validate the msg checksum.
	const auto checksum = msg.get<uint32_t>();
	if (checksum != 0) {
		// Invalid checksum, abort the process.
		close_and_shutdown();
		return;
	}

	// Extract and validate the msg ID (should be 1 for a login request).
	const auto id = msg.get<uint8_t>();
	if (id != 1) {
		// Unexpected msg type, abort.
		close_and_shutdown();
		return;
	}

	// Process the login msg content.
	const auto os = static_cast<OperatingSystem_t>(msg.get<uint16_t>());
	const auto version = msg.get<uint16_t>();

	msg.advance(4);

	if (msg.getRemainingBufferLength() > 132) {
		msg.getString();
	}

	msg.advance(3); // U16 dat revision, U8 preview state

	// Disconnect if RSA decrypt fails
	if (!RSA_decrypt(msg)) {
		close_and_shutdown();
		return;
	}

	// Get XTEA key
	xtea::key key;
	key[0] = msg.get<uint32_t>();
	key[1] = msg.get<uint32_t>();
	key[2] = msg.get<uint32_t>();
	key[3] = msg.get<uint32_t>();
	this->key = std::move(key);

	// Enable extended opcode feature for otclient
	if (os >= CLIENTOS_OTCLIENT_LINUX) {
		NetworkMessage opcodeMessage;
		opcodeMessage.addByte(0x32);
		opcodeMessage.addByte(0x00);
		opcodeMessage.add<uint16_t>(0x00);
		writeToOutputBuffer(opcodeMessage);
	}

	// Change packet verifying mode for QT clients
	if (version >= 1111 && os >= CLIENTOS_QT_LINUX && os <= CLIENTOS_OTCLIENT_MAC) {
		setChecksumMode(CHECKSUM_SEQUENCE);
	}

	// Web login skips the character list request so we need to check the client version again
	if (version < CLIENT_VERSION_MIN || version > CLIENT_VERSION_MAX) {
		close(fmt::format("Only clients with protocol {:s} allowed!", CLIENT_VERSION_STR));
		return;
	}

	msg.skipBytes(1); // Gamemaster flag

	auto sessionToken = tfs::base64::decode(msg.getString());
	if (sessionToken.empty()) {
		close("Malformed session key.");
		return;
	}

	if (os == CLIENTOS_QT_LINUX) {
		msg.getString(); // OS name (?)
		msg.getString(); // OS version (?)
	}

	auto characterName = msg.getString();
	uint32_t timeStamp = msg.get<uint32_t>();
	uint8_t randNumber = msg.getByte();
	if (challenge.timestamp != timeStamp || challenge.random_number != randNumber) {
		close_and_shutdown();
		return;
	}

	if (g_game.getGameState() == GAME_STATE_STARTUP) {
		close("Gameworld is starting up. Please wait.");
		return;
	}

	if (g_game.getGameState() == GAME_STATE_MAINTAIN) {
		close("Gameworld is under maintenance. Please re-connect in a while.");
		return;
	}

	auto ip = getIP();
	if (const auto& banInfo = IOBan::getIpBanInfo(ip)) {
		close(fmt::format("Your IP has been banned until {:s} by {:s}.\n\nReason specified:\n{:s}",
		                  formatDateShort(banInfo->expiresAt), banInfo->bannedBy, banInfo->reason));
		return;
	}

	Database& db = Database::getInstance();
	auto result = db.storeQuery(fmt::format(
	    "SELECT `a`.`id` AS `account_id`, INET6_NTOA(`s`.`ip`) AS `session_ip`, `p`.`id` AS `character_id` FROM `accounts` `a` JOIN `sessions` `s` ON `a`.`id` = `s`.`account_id` JOIN `players` `p` ON `a`.`id` = `p`.`account_id` WHERE `s`.`token` = {:s} AND `s`.`expired_at` IS NULL AND `p`.`name` = {:s} AND `p`.`deletion` = 0",
	    db.escapeString(sessionToken), db.escapeString(characterName)));
	if (!result) {
		close("Account name or password is not correct.");
		return;
	}

	uint32_t accountId = result->getNumber<uint32_t>("account_id");
	if (accountId == 0) {
		close("Account name or password is not correct.");
		return;
	}

	Connection::Address sessionIP = boost::asio::ip::make_address(result->getString("session_ip"));
	if (!sessionIP.is_loopback() && ip != sessionIP) {
		close("Your game session is already locked to a different IP. Please log in again.");
		return;
	}

	g_dispatcher.addTask([=, self = shared_from_this(), characterId = result->getNumber<uint32_t>("character_id")]() {
		Player* foundPlayer = g_game.getPlayerByGUID(characterId);
		if (!foundPlayer || getBoolean(ConfigManager::ALLOW_CLONES)) {
			auto player = new Player(self);
			self->player = player;

			player->incrementReferenceCounter();
			player->setID();
			player->setGUID(characterId);

			if (!IOLoginData::preloadPlayer(player)) {
				close("Your character could not be loaded.");
				return;
			}

			if (IOBan::isPlayerNamelocked(player->getGUID())) {
				close("Your character has been namelocked.");
				return;
			}

			if (g_game.getGameState() == GAME_STATE_CLOSING && !player->hasFlag(PlayerFlag_CanAlwaysLogin)) {
				close("The game is just going down.\nPlease try again later.");
				return;
			}

			if (g_game.getGameState() == GAME_STATE_CLOSED && !player->hasFlag(PlayerFlag_CanAlwaysLogin)) {
				close("Server is currently closed.\nPlease try again later.");
				return;
			}

			if (getBoolean(ConfigManager::ONE_PLAYER_ON_ACCOUNT) &&
			    player->getAccountType() < ACCOUNT_TYPE_GAMEMASTER && g_game.getPlayerByAccount(player->getAccount())) {
				close("You may only login with one character\nof your account at the same time.");
				return;
			}

			if (!player->hasFlag(PlayerFlag_CannotBeBanned)) {
				if (const auto& banInfo = IOBan::getAccountBanInfo(accountId)) {
					if (banInfo->expiresAt > 0) {
						close(fmt::format("Your account has been banned until {:s} by {:s}.\n\nReason specified:\n{:s}",
						                  formatDateShort(banInfo->expiresAt), banInfo->bannedBy, banInfo->reason));
					} else {
						close(
						    fmt::format("Your account has been permanently banned by {:s}.\n\nReason specified:\n{:s}",
						                banInfo->bannedBy, banInfo->reason));
					}
					return;
				}
			}

			if (std::size_t currentSlot = clientLogin(*player)) {
				uint8_t retryTime = getWaitTime(currentSlot);
				auto output = tfs::net::make_output_message();
				output->addByte(0x16);
				output->addString(
				    fmt::format("Too many players online.\nYou are at place {:d} on the waiting list.", currentSlot));
				output->addByte(retryTime);
				send(output);

				disconnect();
				return;
			}

			if (!IOLoginData::loadPlayerById(player, player->getGUID())) {
				close("Your character could not be loaded.");
				return;
			}

			player->setOperatingSystem(operatingSystem);

			if (!g_game.placeCreature(player, player->getLoginPosition())) {
				if (!g_game.placeCreature(player, player->getTemplePosition(), false, true)) {
					close("Temple position is wrong. Contact the administrator.");
					return;
				}
			}

			if (operatingSystem >= CLIENTOS_OTCLIENT_LINUX) {
				player->registerCreatureEvent("ExtendedOpcode");
			}

			player->lastIP = player->getIP();
			player->lastLoginSaved = std::max<time_t>(time(nullptr), player->lastLoginSaved + 1);

			// Proceed to the next step: reading the subsequent header.
			self->read_subsequent_header();
			self->add_auto_flush();
			return;
		}

		if (eventConnect != 0 || !getBoolean(ConfigManager::REPLACE_KICK_ON_LOGIN)) {
			// Already trying to connect
			close("You are already logged in.");
			return;
		}

		if (foundPlayer->connection) {
			foundPlayer->disconnect();
			foundPlayer->isConnecting = true;

			eventConnect = g_scheduler.addEvent(
			    createSchedulerTask(1000, [=, thisPtr = getThis(), playerID = foundPlayer->getID()]() {
				    thisPtr->connect(playerID, operatingSystem);
			    }));
		} else {
			connect(foundPlayer->getID(), operatingSystem);
		}
	});
}

void Connection::read_subsequent_header()
{
	std::lock_guard<std::recursive_mutex> lock(mtx);

	if (disconnected) {
		return;
	}

	// Set a generous timeout for reading subsequent packets.
	stream.expires_after(30s);

	// Reset the msg buffer before reading the next header.
	msg.reset();

	// Read the next 2-byte header to determine the length of the incoming msg.
	async_read(stream, asio::buffer(msg.as_bytes(), 2),
	           [self = shared_from_this()](beast::error_code ec, size_t bytes_transferred) {
		           self->on_read_subsequent_header(ec, bytes_transferred);
	           });
}

void Connection::on_read_subsequent_header(beast::error_code ec, size_t bytes_transferred)
{
	std::lock_guard<std::recursive_mutex> lock(mtx);

	if (ec) {
		fmt::print(stderr, "{}: {}\n", __FUNCTION__, ec.message());
		close_and_shutdown();
		return;
	}

	if (disconnected) {
		return;
	}

	// Disable timeout now that header read is successful.
	stream.expires_never();

	// Extract the length of the subsequent msg from the header.
	auto len = msg.header_len();
	if (len == 0 || len > 24 * 1024) {
		// Reject zero-length or overly large packets (basic sanity check).
		return;
	}

	// Resize the message buffer to fit the payload.
	msg.truncate(len);

	// Continue with reading the rest of the subsequent msg body.
	read_subsequent_body();
}

void Connection::read_subsequent_body()
{
	std::lock_guard<std::recursive_mutex> lock(mtx);

	if (disconnected) {
		return;
	}

	// Apply a generous timeout for reading the subsequent msg body.
	stream.expires_after(30s);

	// Read the remainder of the subsequent msg based on the previously received header length.
	async_read(stream, asio::buffer(msg.body(), msg.header_len()),
	           [self = shared_from_this()](beast::error_code ec, size_t bytes_transferred) {
		           self->on_read_subsequent_body(ec, bytes_transferred);
	           });
}

void Connection::on_read_subsequent_body(beast::error_code ec, size_t bytes_transferred)
{
	std::lock_guard<std::recursive_mutex> lock(mtx);

	if (ec) {
		fmt::print(stderr, "{}: {}\n", __FUNCTION__, ec.message());
		close_and_shutdown();
		return;
	}

	if (disconnected) {
		return;
	}

	// Disable the timeout now that the body has been read.
	stream.expires_never();

	const auto checksum = msg.get<uint32_t>();
	if (checksum == 0) {
		// Invalid checksum, abort the process.
		close_and_shutdown();
		return;
	}

	// Extract the msg ID from the payload.
	const auto id = msg.get<uint8_t>();

	if (!player) {
		if (id == 0x0F) {
			close();
		}

		read_subsequent_header();
		return;
	}

	// a dead player can not performs actions
	if (player->isRemoved() || player->isDead()) {
		if (id == 0x0F) {
			close();
			return;
		}

		if (id != 0x14) {
			read_subsequent_header();
			return;
		}
	}

	if (XTEA_decrypt(msg, key)) {
		read_subsequent(id);
	}

	read_subsequent_header();
}

void Connection::read_subsequent(uint8_t id) {}

void Connection::enqueue(std::shared_ptr<OutgoingBuffer> buffer)
{
	std::lock_guard<std::recursive_mutex> lock(mtx);

	if (disconnected) {
		return;
	}

	buffers.emplace_back(std::move(buffer));

	const auto pending_flush = buffers.size() == 1;
	if (pending_flush) {
		asio::post(stream.get_executor(), [self = shared_from_this()] { self->flush(); });
	}
}

void Connection::flush()
{
	std::lock_guard<std::recursive_mutex> lock(mtx);

	assert(!buffers.empty());

	auto buffer = buffers.front();

	stream.expires_after(30s);

	asio::async_write(stream, asio::buffer(buffer->getOutputBuffer(), buffer->len()),
	                  [self = shared_from_this()](beast::error_code ec, size_t bytes_transferred) {
		                  self->on_flush(ec, bytes_transferred);
	                  });
}

void Connection::on_flush(beast::error_code ec, size_t bytes_transferred)
{
	std::lock_guard<std::recursive_mutex> lock(mtx);

	if (ec) {
		fmt::print(stderr, "{}: {}\n", __FUNCTION__, ec.message());
		close_and_shutdown();
		return;
	}

	stream.expires_never();

	buffers.pop_front();

	const auto pending_flush = !buffers.empty();
	if (pending_flush) {
		flush();
	} else if (disconnected) {
		shutdown();
	}
}

void Connection::shutdown()
{
	std::lock_guard<std::recursive_mutex> lock(mtx);

	if (stream.socket().is_open()) {
		beast::error_code ec;
		stream.socket().shutdown(asio::ip::tcp::socket::shutdown_both, ec);
		stream.socket().close(ec);
	}
}

void Connection::enqueue(std::span<uint8_t> span)
{
	const auto n = span.size();

	if (!current_buffer) {
		current_buffer = make_outgoing_buffer();
	} else if ((current_buffer->len() + n) > 24 * 1024) {
		enqueue(current_buffer);
		current_buffer = make_outgoing_buffer();
	}

	current_buffer->append(span);
}

void Connection::add_auto_flush()
{
	if (auto_flush_connections.empty()) {
		auto_flush(auto_flush_connections);
	}

	auto_flush_connections.emplace_back(shared_from_this());
}

void Connection::remove_auto_flush()
{
	auto it = std::find(auto_flush_connections.begin(), auto_flush_connections.end(), shared_from_this());
	if (it != auto_flush_connections.end()) {
		std::swap(*it, auto_flush_connections.back());
		auto_flush_connections.pop_back();
	}
}

void Connection::close(std::string_view msg)
{
	write_login_error(msg);
	close();
}

void Connection::close()
{
	std::lock_guard<std::recursive_mutex> lock(mtx);

	disconnected = true;

	g_dispatcher.addTask([self = shared_from_this()]() {
		if (const auto player = g_game.getPlayerByID(self->get_player_id())) {
			if (player->connection == self) {
				player->connection = nullptr;
				player = nullptr;
			}
		}

		self->remove_auto_flush();
	});
}

void Connection::close_and_shutdown()
{
	std::lock_guard<std::recursive_mutex> lock(mtx);

	close();
	shutdown();
}

std::shared_ptr<Connection> make_connection(asio::ip::tcp::socket&& socket)
{
	return std::make_shared<Connection>(std::move(socket));
}

} // namespace tfs::game_server
