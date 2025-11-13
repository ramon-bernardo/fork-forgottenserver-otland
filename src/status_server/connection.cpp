#include "connection.h"

#include "packet.h"

#include <boost/asio.hpp>
#include <boost/asio/dispatch.hpp>
#include <fmt/core.h>
#include "connection.h"

namespace tfs::status_server {

Connection::Connection(asio::ip::tcp::socket&& socket) : stream{std::move(socket)}
{
	beast::error_code ec;
	if (const auto endpoint = stream.socket().remote_endpoint(); !ec) {
		address = endpoint.address();
	}
}

void Connection::read()
{
	using namespace std::chrono_literals;

	// Set the timeout.
	stream.expires_after(30s);

	// Read a request
	stream.async_read_some(buffer.prepare(512),
	                       [self = shared_from_this()](beast::error_code ec, size_t bytes_transferred) {
		                       self->on_read(ec, bytes_transferred);
	                       });
}

void Connection::write(beast::flat_buffer buffer)
{
	// Write the response
	async_write(stream, std::move(buffer), [self = shared_from_this()](beast::error_code ec, size_t bytes_transferred) {
		self->on_write(ec, bytes_transferred);
	});
}

void Connection::close()
{
	// Send a TCP shutdown
	beast::error_code ec;
	stream.socket().shutdown(asio::ip::tcp::socket::shutdown_both, ec);

	// At this point the connection is closed gracefully
}

void Connection::run()
{
	// We need to be executing within a strand to perform async operations
	// on the I/O objects in this session. Although not strictly necessary
	// for single-threaded contexts, this example code is written to be
	// thread-safe by default.
	dispatch(stream.get_executor(), [self = shared_from_this()] { self->read(); });
}

void Connection::on_read(beast::error_code ec, size_t bytes_transferred)
{
	if (ec == asio::error::eof) {
		close();
		return;
	}

	if (ec) {
		fmt::print(stderr, "{}: {}\n", __FUNCTION__, ec.message());
		return;
	}

	buffer.commit(bytes_transferred);

	write(handle_packet(std::move(buffer)));
}

void Connection::on_write(beast::error_code ec, size_t /*bytes_transferred*/)
{
	if (ec) {
		fmt::print(stderr, "{}: {}\n", __FUNCTION__, ec.message());
		return;
	}

	close();
}

std::shared_ptr<Connection> make_connection(asio::ip::tcp::socket&& socket)
{
	return std::make_shared<Connection>(std::move(socket));
}

} // namespace tfs::status_server
