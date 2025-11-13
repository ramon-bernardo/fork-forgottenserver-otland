#include "../position.h"

#include <string>
#include <array>

namespace tfs::game_server {

/**
 * @brief A class for handling incoming network messages with sequential binary reading.
 *
 * This class provides mechanisms for reading binary data sequentially from a message buffer.
 * It manages a fixed-size internal buffer and maintains a read position (`pos`) that advances as data is read.
 */
class IncomingMessage
{
public:
	IncomingMessage() = default;

	/**
	 * @brief Reads a trivially copyable value from the current cursor position.
	 *
	 * Advances the cursor by the size of the type read.
	 *
	 * @tparam T The type to read (must be trivially copyable).
	 * @return The value read from the buffer.
	 */
	template <typename T>
	T get() noexcept;

	/**
	 * @brief Reads a string from the buffer.
	 *
	 * This function reads a UTF-8 encoded string from the current position.
	 * It first reads a 16-bit length prefix, then reads that many bytes as
	 * an ISO-8859-1 string and converts it to UTF-8.
	 *
	 * @return The string read from the buffer.
	 */
	template <>
	std::string get<std::string>() noexcept;

	/**
	 * @brief Reads a Position structure from the buffer.
	 *
	 * This function reads the `x`, `y`, and `z` components of a `Position` object
	 * sequentially from the buffer, advancing the internal cursor accordingly.
	 *
	 * @return The `Position` object read from the buffer.
	 */
	template <>
	Position get<Position>() noexcept;

	/**
	 * @brief Reads a trivially copyable value from the previous cursor position.
	 *
	 * Useful when you need to re-read the last element fetched.
	 *
	 * @tparam T The type to read.
	 * @return The value read from the previous position.
	 */
	template <typename T>
	T get_prev() noexcept;

	/**
	 * @brief Returns a pointer to the remaining unread portion of the message body.
	 *
	 * @return Pointer to the start of unread data.
	 */
	auto body() { return &buffer[pos]; }

	/**
	 * @brief Gets the length of the message header.
	 *
	 * @return The message length in bytes.
	 */
	auto header_len() const { return static_cast<uint16_t>(buffer[0] | buffer[1] << 8); }

	/**
	 * @brief Sets the message length in the header.
	 *
	 * @param n The total message length.
	 */
	void set_header_len(uint16_t n)
	{
		buffer[0] = static_cast<uint8_t>(n & 0xFF);
		buffer[1] = static_cast<uint8_t>((n >> 8) & 0xFF);
		length = n;
	}

	/**
	 * @brief Returns the number of bytes remaining to be read.
	 *
	 * @return Remaining bytes.
	 */
	auto remaining() const { return static_cast<uint16_t>(length - pos); }

	/**
	 * @brief Returns a pointer to the full byte array.
	 *
	 * @return Pointer to the byte array.
	 */
	auto as_bytes() { return &buffer[0]; }

	/**
	 * @brief Returns a const pointer to the full byte array.
	 *
	 * @return Const pointer to the byte array.
	 */
	const auto as_bytes() const { return &buffer[0]; }

	/**
	 * @brief Returns a pointer to the slice starting from the current cursor position.
	 *
	 * @return Pointer to remaining data.
	 */
	auto remaining_slice() { return &buffer[0] + pos; }

	/**
	 * @brief Returns a const pointer to the slice starting from the current cursor position.
	 *
	 * @return Const pointer to remaining data.
	 */
	const auto remaining_slice() const { return &buffer[0] + pos; }

	/**
	 * @brief Gets the current read cursor position.
	 *
	 * @return The current position.
	 */
	auto position() const { return pos; }

	/**
	 * @brief Moves the read cursor to a specific position.
	 *
	 * @param n The new position.
	 */
	void seek_to(uint16_t n) { pos = n; }

	/**
	 * @brief Advances the read cursor by a specified number of bytes.
	 *
	 * @param n Number of bytes to move forward.
	 */
	void advance(int16_t n) { pos += n; }

	/**
	 * @brief Truncates the message length to n bytes (excluding header).
	 *
	 * @param n New message body length.
	 */
	void truncate(uint16_t n) { length = n + 2; }

	/**
	 * @brief Gets the total message length.
	 *
	 * @return The message length.
	 */
	auto len() const { return length; }

	/**
	 * @brief Checks if the message is empty.
	 *
	 * @return True if empty, false otherwise.
	 */
	auto is_empty() const { return length == 0; }

	/**
	 * @brief Resets the internal state.
	 */
	void reset() noexcept
	{
		pos = 2;
		length = 0;
	}

private:
	/// Current read cursor (starts after header)
	uint16_t pos = 2;

	/// Total message length including header
	uint16_t length = 0;

	/// Internal buffer of fixed size
	std::array<uint8_t, 24 * 1024> buffer;
};

} // namespace tfs::game_server
