#pragma once

#include <boost/beast/core.hpp>

namespace beast = boost::beast;

namespace tfs::status_server {

beast::flat_buffer handle_xml();

}
