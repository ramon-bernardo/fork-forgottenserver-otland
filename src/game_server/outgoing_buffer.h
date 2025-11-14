#include <array>
#include <memory>
#include <span>

namespace tfs::game_server {

class OutgoingBuffer
{
public:
	OutgoingBuffer() = default;

	auto len() const { return length; }
	auto body() { return &buffer[start]; }

	void append(std::span<uint8_t> span)
	{
		const auto n = span.size();
		std::memcpy(buffer.data() + pos, span.data(), n);
		length += n;
		pos += n;
	}

private:
	/// Current read cursor
	uint16_t pos = 8;

	/// Total message length including header
	uint16_t length = 0;

	uint16_t start = 8;

	/// Internal buffer of fixed size
	std::array<uint8_t, 24 * 1024> buffer;
};

std::shared_ptr<OutgoingBuffer> make_outgoing_buffer();

} // namespace tfs::game_server
