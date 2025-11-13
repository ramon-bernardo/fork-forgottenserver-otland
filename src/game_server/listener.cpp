#include "listener.h"

#include "connection.h"

#include <boost/asio/strand.hpp>
#include <fmt/core.h>

namespace tfs::game_server {

void Listener::accept()
{
	// The new connection gets its own strand
	acceptor.async_accept(asio::make_strand(ioc),
	                      [self = shared_from_this()](beast::error_code ec, asio::ip::tcp::socket socket) {
		                      self->on_accept(ec, std::move(socket));
	                      });
}

void Listener::on_accept(beast::error_code ec, asio::ip::tcp::socket socket)
{
	if (ec) {
		fmt::print(stderr, "{}: {}\n", __FUNCTION__, ec.message());
		return;
	}

	// Create the connection and run it
	auto connection = make_connection(std::move(socket));
	connection->run();

	// Accept another connection
	accept();
}

std::shared_ptr<Listener> make_listener(asio::io_context& iocontext, asio::ip::tcp::endpoint endpoint)
{
	asio::ip::tcp::acceptor acceptor{asio::make_strand(iocontext)};

	beast::error_code ec;
	if (acceptor.set_option(asio::ip::tcp::no_delay{true}, ec); ec) {
		throw std::runtime_error(ec.message());
	}

	return std::make_shared<Listener>(iocontext, std::move(acceptor));
}

} // namespace tfs::game_server
