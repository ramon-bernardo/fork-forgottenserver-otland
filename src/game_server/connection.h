#pragma once

#include "../player.h"
#include "../xtea.h"
#include "reader.h"
#include "writer.h"

#include <boost/asio/ip/tcp.hpp>
#include <boost/beast/core/flat_buffer.hpp>
#include <boost/beast/core/tcp_stream.hpp>
#include <memory>
#include <zlib.h>

namespace asio = boost::asio;
namespace beast = boost::beast;
namespace system = boost::system;

namespace tfs::game_server {

class Connection final : public std::enable_shared_from_this<Connection>, public Writer, public Reader
{
public:
	Connection(asio::ip::tcp::socket&& socket);
	~Connection();

	void run();
	void write(std::shared_ptr<OutgoingMessage> msg);
	void close(std::string_view msg);
	void close();
	void close_and_shutdown();

private:
	void read_server_name();
	void on_read_server_name(beast::error_code ec, size_t bytes_transferred);

	void read_login_header();
	void on_read_login_header(beast::error_code ec, size_t bytes_transferred);
	void read_login_body();
	void on_read_login_body(beast::error_code ec, size_t bytes_transferred);

	void read_subsequent_header();
	void on_read_subsequent_header(beast::error_code ec, size_t bytes_transferred);
	void read_subsequent_body();
	void on_read_subsequent_body(beast::error_code ec, size_t bytes_transferred);

	void flush();
	void on_flush(beast::error_code ec, size_t bytes_transferred);

	void shutdown();
	uint32_t get_player_id() const override { return player ? player->getID() : 0; }

	void add_auto_flush();
	void remove_auto_flush();

	std::recursive_mutex mtx;
	beast::tcp_stream stream;

	IncomingMessage msg;
	std::list<OutgoingMessage> msgs;

	struct Challenge
	{
		uint32_t timestamp;
		uint8_t random_number;
	} challenge;

	Player* player = nullptr;
	xtea::round_keys key;

	bool disconnected = false;
};

std::shared_ptr<Connection> make_connection(asio::ip::tcp::socket&& socket);

} // namespace tfs::game_server
