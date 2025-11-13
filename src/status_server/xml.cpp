#include "xml.h"

#include "../configmanager.h"

#include <pugixml.hpp>

namespace asio = boost::asio;

beast::flat_buffer tfs::status_server::handle_xml()
{
	pugi::xml_document doc;

	pugi::xml_node decl = doc.prepend_child(pugi::node_declaration);
	decl.append_attribute("version") = "1.0";

	pugi::xml_node tsqp = doc.append_child("tsqp");
	tsqp.append_attribute("version") = "1.0";

	pugi::xml_node serverinfo = tsqp.append_child("serverinfo");
	serverinfo.append_attribute("uptime") = std::to_string(tfs::status_server::uptime()).c_str();
	serverinfo.append_attribute("ip") = getString(ConfigManager::IP).c_str();
	serverinfo.append_attribute("servername") = getString(ConfigManager::SERVER_NAME).c_str();
	serverinfo.append_attribute("port") = std::to_string(getNumber(ConfigManager::LOGIN_PORT)).c_str();
	serverinfo.append_attribute("location") = getString(ConfigManager::LOCATION).c_str();
	serverinfo.append_attribute("url") = getString(ConfigManager::URL).c_str();
	serverinfo.append_attribute("server") = STATUS_SERVER_NAME;
	serverinfo.append_attribute("version") = STATUS_SERVER_VERSION;
	serverinfo.append_attribute("client") = CLIENT_VERSION_STR;

	pugi::xml_node owner = tsqp.append_child("owner");
	owner.append_attribute("name") = getString(ConfigManager::OWNER_NAME).c_str();
	owner.append_attribute("email") = getString(ConfigManager::OWNER_EMAIL).c_str();

	pugi::xml_node players = tsqp.append_child("players");

	uint32_t reportableOnlinePlayerCount = 0;
	uint32_t maxPlayersPerIp = getNumber(ConfigManager::STATUS_COUNT_MAX_PLAYERS_PER_IP);
	if (maxPlayersPerIp > 0) {
		std::map<Connection::Address, uint32_t> playersPerIp;
		for (const auto& it : g_game.getPlayers()) {
			if (!it.second->getIP().is_unspecified()) {
				++playersPerIp[it.second->getIP()];
			}
		}

		for (auto& p : playersPerIp | std::views::values) {
			reportableOnlinePlayerCount += std::min(p, maxPlayersPerIp);
		}
	} else {
		reportableOnlinePlayerCount = g_game.getPlayersOnline();
	}

	players.append_attribute("online") = std::to_string(reportableOnlinePlayerCount).c_str();
	players.append_attribute("max") = std::to_string(getNumber(ConfigManager::MAX_PLAYERS)).c_str();
	players.append_attribute("peak") = std::to_string(g_game.getPlayersRecord()).c_str();

	pugi::xml_node monsters = tsqp.append_child("monsters");
	monsters.append_attribute("total") = std::to_string(g_game.getMonstersOnline()).c_str();

	pugi::xml_node npcs = tsqp.append_child("npcs");
	npcs.append_attribute("total") = std::to_string(g_game.getNpcsOnline()).c_str();

	pugi::xml_node rates = tsqp.append_child("rates");
	rates.append_attribute("experience") = std::to_string(getNumber(ConfigManager::RATE_EXPERIENCE)).c_str();
	rates.append_attribute("skill") = std::to_string(getNumber(ConfigManager::RATE_SKILL)).c_str();
	rates.append_attribute("loot") = std::to_string(getNumber(ConfigManager::RATE_LOOT)).c_str();
	rates.append_attribute("magic") = std::to_string(getNumber(ConfigManager::RATE_MAGIC)).c_str();
	rates.append_attribute("spawn") = std::to_string(getNumber(ConfigManager::RATE_SPAWN)).c_str();

	pugi::xml_node map = tsqp.append_child("map");
	map.append_attribute("name") = getString(ConfigManager::MAP_NAME).c_str();
	map.append_attribute("author") = getString(ConfigManager::MAP_AUTHOR).c_str();

	uint32_t mapWidth, mapHeight;
	g_game.getMapDimensions(mapWidth, mapHeight);
	map.append_attribute("width") = std::to_string(mapWidth).c_str();
	map.append_attribute("height") = std::to_string(mapHeight).c_str();

	pugi::xml_node motd = tsqp.append_child("motd");
	motd.text() = "N/A";

	std::ostringstream ss;
	doc.save(ss, "", pugi::format_raw);

	std::string data = ss.str();

	beast::flat_buffer buffer;
	buffer.commit(asio::buffer_copy(buffer.prepare(data.size()), asio::buffer(data)));
	return buffer;
}
