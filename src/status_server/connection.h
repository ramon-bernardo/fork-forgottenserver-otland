#pragma once

#include "../outputmessage.h"

#include <boost/asio.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/beast/core/flat_buffer.hpp>
#include <boost/beast/core/tcp_stream.hpp>
#include <memory>
#include <random>

namespace asio = boost::asio;
namespace beast = boost::beast;

namespace tfs::status_server {

class Connection final : public std::enable_shared_from_this<Connection>
{
public:
	Connection(asio::ip::tcp::socket&& socket);

	void read();
	void write(beast::flat_buffer buffer);
	void close();
	void run();

private:
	void on_read(beast::error_code ec, size_t bytes_transferred);
	void on_write(beast::error_code ec, size_t bytes_transferred);

	beast::flat_buffer buffer;
	beast::tcp_stream stream;
	asio::ip::address address;
};

std::shared_ptr<Connection> make_connection(asio::ip::tcp::socket&& socket);

} // namespace tfs::status_server
