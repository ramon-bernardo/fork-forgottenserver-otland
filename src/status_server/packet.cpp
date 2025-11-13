#include "packet.h"

#include "message.h"
#include "xml.h"

#include <boost/beast/core.hpp>
#include <boost/locale.hpp>
#include <string>
#include <string_view>

namespace locale = boost::locale;

namespace {

template <typename T>
std::enable_if_t<std::is_trivially_copyable_v<T>, T> read(const beast::flat_buffer& buffer)
{
	static_assert(std::is_trivially_constructible_v<T>, "Destination type must be trivially constructible");

	if (buffer.size() < sizeof(T)) {
		throw std::runtime_error("Buffer underflow");
	}

	T value;
	std::memcpy(&value, buffer.data(), sizeof(T));
	buffer.consume(sizeof(T));
	return value;
}

std::string read_string(const beast::flat_buffer& buffer, uint16_t n = 0)
{
	if (n == 0) {
		n = read<uint16_t>(buffer);
	}

	if (buffer.size() < n) {
		throw std::runtime_error("Buffer underflow");
	}

	std::string_view s(static_cast<const char*>(buffer.data().data()), n);
	// buffer.consume(n);

	return locale::conv::to_utf<char>(s.data(), s.data() + s.size(), "ISO-8859-1", locale::conv::skip);
}

} // namespace

beast::flat_buffer tfs::status_server::handle_packet(const beast::flat_buffer& buffer)
{
	const auto checksum = read<uint32_t>(buffer);

	const auto id = read<uint8_t>(buffer);
	if (id != 0xFF) {
		throw std::runtime_error("Unknown status server id.");
	}

	const auto type = read<uint8_t>(buffer);
	if (type == 0xFF) {
		const auto information = read_string(buffer, 4) == "info";
		if (information) {
			return handle_xml();
		}
	}

	if (type == 0x01) {
		const auto flags = read<uint16_t>(buffer);
		const auto playerName = flags & Flags::PlayerStatus ? read_string(buffer) : std::string();
		return handle_message(flags, std::move(playerName));
	}

	throw std::runtime_error("Invalid request.");
}
