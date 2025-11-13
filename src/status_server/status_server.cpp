#include "status_server.h"

#include "../tools.h"
#include "listener.h"

#include <fmt/core.h>

namespace asio = boost::asio;

namespace {

const auto start_time = OTSYS_TIME();

asio::io_context ioc;
std::thread worker;

} // namespace

void tfs::status_server::start(bool bind_only_ip, std::string_view ip, unsigned short port)
{
	if (port == 0) {
		return;
	}

	asio::ip::address address = bind_only_ip ? asio::ip::make_address(ip) : asio::ip::address_v6::any();
	fmt::print(">> Starting STATUS server on {:s}:{:d}.\n", address.to_string(), port);

	auto listener = make_listener(ioc, {address, port});
	listener->run();

	worker = std::thread([] { ioc.run(); });
}

void tfs::status_server::stop()
{
	fmt::print(">> Stopping STATUS server...\n");

	ioc.stop();
	worker.join();

	fmt::print(">> Stopped STATUS server.\n");
}

auto tfs::status_server::uptime() { return (OTSYS_TIME() - start_time) / 1000; }
