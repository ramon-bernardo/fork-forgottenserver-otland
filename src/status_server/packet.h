#pragma once

#include <boost/asio/ip/tcp.hpp>
#include <boost/beast/core/flat_buffer.hpp>

namespace asio = boost::asio;
namespace beast = boost::beast;

namespace tfs::status_server {

enum Flags
{
	BasicServer = 1 << 0,
	OwnerServer = 1 << 1,
	MiscServer = 1 << 2,
	Players = 1 << 3,
	Map = 1 << 4,
	ExtPlayers = 1 << 5,
	PlayerStatus = 1 << 6,
	ServerSoftware = 1 << 7,
};

beast::flat_buffer handle_packet(const beast::flat_buffer& buffer);

} // namespace tfs::status_server
