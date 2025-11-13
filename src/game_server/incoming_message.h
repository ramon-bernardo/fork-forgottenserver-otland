#include <boost/locale.hpp>

namespace locale = boost::locale;

namespace tfs::game_server {

class IncomingMessage
{
public:
	IncomingMessage() = default;

	template <typename T>
	std::enable_if_t<std::is_trivially_copyable_v<T>, T> get() noexcept
	{
		static_assert(std::is_trivially_constructible_v<T>, "Destination type must be trivially constructible");

		if (pos + sizeof(T) > length) {
			return 0;
		}

		T value;
		std::memcpy(&value, buffer.data() + pos, sizeof(T));
		pos += sizeof(T);
		return value;
	}

	template <typename T>
	std::enable_if_t<std::is_trivially_copyable_v<T>, T> get_prev() noexcept
	{
		static_assert(std::is_trivially_constructible_v<T>, "Destination type must be trivially constructible");

		if (pos < sizeof(T)) {
			return 0;
		}

		pos -= sizeof(T);

		T value;
		std::memcpy(&value, buffer.data() + pos, sizeof(T));
		return value;
	}

	std::string get_string(uint16_t n = 0)
	{
		if (n == 0) {
			n = get<uint16_t>();
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

	auto body() { return &buffer[pos]; }

	auto header_len() const { return static_cast<uint16_t>(buffer[0] | buffer[1] << 8); }
	void set_header_len(uint16_t n)
	{
		buffer[0] = static_cast<uint8_t>(n & 0xFF);
		buffer[1] = static_cast<uint8_t>((n >> 8) & 0xFF);
		length = n;
	}

	auto remaining() const { return static_cast<uint16_t>(length - pos); }

	auto as_bytes() { return &buffer[0]; }
	const auto as_bytes() const { return &buffer[0]; }

	auto remaining_slice() { return &buffer[0] + pos; }
	const auto remaining_slice() const { return &buffer[0] + pos; }

	auto position() const { return pos; }
	void seek_to(uint16_t n) { pos = n; }
	void advance(int16_t n) { pos += n; }

	void truncate(uint16_t n) { length = n + 2; }
	auto len() const { return length; }
	auto is_empty() const { return length == 0; }

	void reset() noexcept
	{
		pos = 2;
		length = 0;
	}

private:
	uint16_t pos = 2;
	uint16_t length = 0;
	std::array<uint8_t, 24 * 1024> buffer;
};

} // namespace tfs::game_server
//
// class NetworkMessage
//{
// public:
//	using MsgSize_t = uint16_t;
//	// Headers:
//	// 2 bytes for unencrypted message size
//	// 4 bytes for checksum
//	// 2 bytes for encrypted message size
//	static constexpr MsgSize_t INITIAL_BUFFER_POSITION = 8;
//
//	NetworkMessage() = default;
//
//	void reset() { info = {}; }
//
//	std::string getString(uint16_t stringLen = 0);
//	Position getPosition();
//
//	bool setBufferPosition(MsgSize_t pos)
//	{
//		if (pos < NETWORKMESSAGE_MAXSIZE - INITIAL_BUFFER_POSITION) {
//			info.position = pos + INITIAL_BUFFER_POSITION;
//			return true;
//		}
//		return false;
//	}
//
//	bool isOverrun() const { return info.overrun; }
//
// protected:
//	struct NetworkMessageInfo
//	{
//		MsgSize_t length = 0;
//		MsgSize_t position = INITIAL_BUFFER_POSITION;
//		bool overrun = false;
//	};
//
//	NetworkMessageInfo info;
//	std::array<uint8_t, NETWORKMESSAGE_MAXSIZE> buffer;
//
// private:
//	bool canAdd(size_t size) const { return (size + info.position) < MAX_BODY_LENGTH; }
//
//	bool canRead(int32_t size)
//	{
//		if ((position + size) > (length + 8) || size >= (NETWORKMESSAGE_MAXSIZE - position)) {
//			info.overrun = true;
//			return false;
//		}
//		return true;
//	}
//};
