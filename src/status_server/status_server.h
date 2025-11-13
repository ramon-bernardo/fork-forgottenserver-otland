#pragma once

#include <string_view>

namespace tfs::status_server {

void start(bool bind_only_ip, std::string_view ip, unsigned short port = 7171);
void stop();
auto uptime();

} // namespace tfs::status_server
