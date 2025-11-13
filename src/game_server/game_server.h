#pragma once

#include <string_view>

namespace tfs::game_server {

void start(bool bind_only_ip, std::string_view ip, unsigned short port = 7171);
void stop();

} // namespace tfs::game_server
