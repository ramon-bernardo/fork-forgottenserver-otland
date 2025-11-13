#pragma once

#include <boost/beast/core.hpp>

namespace beast = boost::beast;

namespace tfs::status_server {

beast::flat_buffer handle_message(uint16_t flags, std::string player_name);

}
