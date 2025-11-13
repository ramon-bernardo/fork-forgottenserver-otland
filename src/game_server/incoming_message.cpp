#include "incoming_message.h"

#include <boost/locale.hpp>

namespace locale = boost::locale;

namespace tfs::game_server {

template <typename T>
T get() noexcept
{
	static_assert(std::is_trivially_constructible_v<T>, "Destination type must be trivially constructible");
	static_assert(std::is_trivially_copyable_v<T>, "Type T must be trivially copyable");

	if (pos + sizeof(T) > length) {
		return T{};
	}

	T value;
	std::memcpy(&value, buffer.data() + pos, sizeof(T));
	pos += sizeof(T);
	return value;
}

template <>
std::string IncomingMessage::get<std::string>() noexcept
{
	const auto n = get<uint16_t>();
	if (n == 0) {
		return {};
	}

	if (pos < n) {
		return {};
	}

	auto it = buffer.data() + pos;
	pos += n;

	std::string_view latin1Str{reinterpret_cast<char*>(it), n};
	return locale::conv::to_utf<char>(latin1Str.data(), latin1Str.data() + latin1Str.size(), "ISO-8859-1",
	                                  locale::conv::skip);
}

template <>
Position IncomingMessage::get<Position>() noexcept
{
	Position pos;
	pos.x = get<uint16_t>();
	pos.y = get<uint16_t>();
	pos.z = get<uint8_t>();
	return pos;
}

template <typename T>
T get_prev() noexcept
{
	static_assert(std::is_trivially_constructible_v<T>, "Destination type must be trivially constructible");
	static_assert(std::is_trivially_copyable_v<T>, "Type T must be trivially copyable");

	if (pos < sizeof(T)) {
		return T{};
	}

	pos -= sizeof(T);

	T value;
	std::memcpy(&value, buffer.data() + pos, sizeof(T));
	return value;
}

} // namespace tfs::game_server
