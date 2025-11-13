
#include "../item.h"
#include "../position.h"

#include <array>
#include <span>
#include <string_view>

namespace tfs::game_server {

class OutgoingMessage
{
public:
	OutgoingMessage() = default;

	template <typename T>
	void add(T value);

	template <>
	void add(std::string_view value);

	template <>
	void add(Position value);

	template <>
	void add(const Item* item);

	std::span<uint8_t> take()
	{
		std::span<uint8_t> result(buffer.data(), length);
		length = 0;
		return result;
	}

private:
	/// Total message length
	uint16_t length = 0;

	/// Internal buffer of fixed size
	std::array<uint8_t, 24 * 1024> buffer;
};

} // namespace tfs::game_server
