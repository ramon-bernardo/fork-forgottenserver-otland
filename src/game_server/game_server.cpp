#include "game_server.h"

#include "listener.h"

#include <fmt/core.h>

namespace asio = boost::asio;

namespace {

asio::io_context ioc;

} // namespace

void tfs::game_server::start(bool bind_only_ip, std::string_view ip, unsigned short port)
{
	if (port == 0) {
		return;
	}

	asio::ip::address address = bind_only_ip ? asio::ip::make_address(ip) : asio::ip::address_v6::any();
	fmt::print(">> Starting GAME server on {:s}:{:d}.\n", address.to_string(), port);

	auto listener = make_listener(ioc, {address, port});
	listener->run();

	ioc.run();
}

void tfs::game_server::stop()
{
	fmt::print(">> Stopping GAME server...\n");

	ioc.stop();

	fmt::print(">> Stopped GAME server.\n");
}
