#pragma once

#include <cstdint>
#include <cstring>
#include <memory>
#include <stdexcept>
#include <string>
#include <string_view>

#include "sajson.hh"

namespace janus
{
// A simple dynamically-sized binary buffer with a fixed capacity.
class dynamic_buffer
{
public:
	// We explicitly require a buffer size in bytes. This will be rounded up
	// to uint64 words.
	explicit dynamic_buffer(uint64_t cap)
		// Note that we store capacity/size values in uint64 words.
		: _cap{align64(cap) / sizeof(uint64_t)},
		  _read_offset{0},
		  _write_offset{0},
		  _buf{std::make_unique<uint64_t[]>(_cap)} // NOLINT: Can't use std::array here.
	{
	}

	// Make the buffer moveable.
	dynamic_buffer(dynamic_buffer&& that) noexcept
		: _cap{that._cap},
		  _read_offset(that._read_offset),
		  _write_offset{that._write_offset},
		  _buf{std::move(that._buf)}
	{
		that._cap = 0;
		that._read_offset = 0;
		that._write_offset = 0;
		that._buf = nullptr;
	}

	// The default dtor is fine, our underlying buffer is wrapped by a unique_ptr.
	~dynamic_buffer() = default;

	// Delete the copy ctor + assignment operator we want to discourage copying.
	dynamic_buffer(const dynamic_buffer&) = delete;
	auto operator=(const dynamic_buffer&) -> dynamic_buffer& = delete;
	// Discourage move assignment as well, we don't want to be overwriting
	// a buffer.
	auto operator=(dynamic_buffer &&) -> dynamic_buffer& = delete;

	// Overall maximum capacity of the buffer in bytes. If this is exceeded, we
	// throw.
	auto cap() const -> uint64_t
	{
		// We store this value as a multiple of uint64_t words
		// internally.
		return _cap * sizeof(uint64_t);
	}

	// Current used bytes, also represents the offset at which data will be
	// written. size() <= cap() is an invariant.
	auto size() const -> uint64_t
	{
		// We store this value as a multiple of uint64_t words
		// internally.
		return _write_offset * sizeof(uint64_t);
	}

	// Read offset into buffer in bytes.
	auto read_offset() const -> uint64_t
	{
		// We store this value as a multiple of uint64_t words
		// internally.
		return _read_offset * sizeof(uint64_t);
	}

	// Return raw underlying buffer.
	auto data() const -> uint64_t*
	{
		return _buf.get();
	}

	// Read a uint64 and advance read offset.
	auto read_uint64() -> uint64_t
	{
		check_read_overflow(1);
		return _buf[_read_offset++];
	}

	// Return a pointer to the the buffer at the current read offset and
	// increment read offset, taking into account 64-bit alignment accordingly.
	//   size: Size in bytes of required block of memory.
	auto read_raw(uint64_t size) -> void*
	{
		uint64_t aligned_bytes = align64(size);
		uint64_t aligned_words = aligned_bytes / sizeof(uint64_t);
		check_read_overflow(aligned_words);

		void* ret = &_buf[_read_offset];
		_read_offset += aligned_words;
		return ret;
	}

	// Return an arbitrary object from the buffer at the read offset.
	template<typename T>
	auto read() -> T&
	{
		return *static_cast<T*>(read_raw(sizeof(T)));
	}

	// Decode and read a string from the buffer. Returns a string view
	// pointing at the char array in the buffer.
	auto read_string() -> std::string_view
	{
		uint64_t count = read_uint64();
		if (count == 0)
			return std::string_view(nullptr, 0);

		// + 1 for the null terminator.
		char* str = static_cast<char*>(read_raw(count + 1));

		return std::string_view(str, count);
	}

	// Add a uint64 value to the buffer.
	void add_uint64(uint64_t n)
	{
		check_write_overflow(1);
		_buf[_write_offset++] = n;
	}

	// Add raw data to the buffer. It will be aligned to word size with
	// remaining bytes zeroed.
	//      ptr: Raw pointer of memory to copy to buffer.
	//     size: Size in bytes of memory to copy.
	//  returns: Raw pointer to allocated memory.
	auto add_raw(const void* ptr, uint64_t size) -> void*
	{
		uint64_t aligned_bytes = align64(size);
		uint64_t aligned_words = aligned_bytes / sizeof(uint64_t);
		check_write_overflow(aligned_words);

		auto* buf = reinterpret_cast<uint8_t*>(&_buf[_write_offset]);
		std::memcpy(buf, ptr, size);
		// It is safe to call this with 0 size.
		// aligned >= size always.
		std::memset(&buf[size], '\0', aligned_bytes - size);
		_write_offset += aligned_words;

		return buf;
	}

	// Add an arbitrary object to the buffer.
	template<typename T>
	auto add(T& ptr) -> T&
	{
		return *static_cast<T*>(add_raw(&ptr, sizeof(T)));
	}

	// Encode a string into binary format and add it to the buffer.
	//     str: Pointer to the char buffer, WITH null termintaor.
	//    size: Size of string WITHOUT null terminator.
	// returns: String view pointing at the string within the dynamic buffer.
	auto add_string(const char* str, uint64_t size) -> std::string_view
	{
		if (size == 0) {
			add_uint64(0);
			return std::string_view(nullptr, 0);
		}

		add_uint64(size);
		void* ptr = add_raw(str, size + 1);
		return std::string_view(static_cast<char*>(ptr), size);
	}

	// Encode an sajson string into binary format and add it to the buffer.
	//    node: sajson::value node.
	// returns: String view pointing at the string within the dynamic buffer.
	auto add_string(const sajson::value& node) -> std::string_view
	{
		return add_string(node.as_cstring(), node.get_string_length());
	}

	// Clear the buffer but maintain the capacity.
	void reset()
	{
		_write_offset = 0;
		// Clearing the buffer invalidates the read offset.
		reset_read();
	}

	// Reset the read offset to the start of the buffer.
	void reset_read()
	{
		_read_offset = 0;
	}

private:
	// Capacity of buffer in uint64 words (i.e. cap_bytes = 8 * _cap).
	uint64_t _cap;
	// Offset tracking a read through this buffer in uint64 words.
	uint64_t _read_offset;
	// Size of buffer in uint64 words (i.e. size_bytes = 8 * _write_offset).
	uint64_t _write_offset;
	// Store our data as an array of uint64_t so we're (64-bit) word aligned
	// by default.
	std::unique_ptr<uint64_t[]> _buf; // NOLINT: Can't use std::array here.

	// Align input value to 64-bits, i.e. 8 bytes.
	static constexpr auto align64(uint64_t size) -> uint64_t
	{
		// Anything not aligned will increase to above the next 8-byte
		// aligned value, then by clearing the trailing bits we obtain
		// that value. Aligned values remain the same.
		return (size + (sizeof(uint64_t) - 1)) & ~(sizeof(uint64_t) - 1);
	}

	// Throw an exception indicating an overflow.
	static void throw_overflow(const char* op, const char* what, uint64_t delta,
				   uint64_t offset, uint64_t size)
	{
		uint64_t delta_bytes = delta * sizeof(uint64_t);
		uint64_t offset_bytes = offset * sizeof(uint64_t);
		uint64_t size_bytes = size * sizeof(uint64_t);

		// We are OK with some string allocations on the exceptional
		// code path.
		throw std::runtime_error(op + std::to_string(delta_bytes) +
					 " bytes from offset of " + std::to_string(offset_bytes) +
					 " bytes, exceeding " + what + " of " +
					 std::to_string(size_bytes));
	}

	// Determine if read would overflow the capacity of the buffer and if
	// so, throw.
	//   delta: Delta in size expressed in uint64 words.
	void check_read_overflow(uint64_t delta)
	{
		if (_read_offset + delta > _write_offset)
			throw_overflow("Requested read of ", "size", delta, _read_offset,
				       _write_offset);
	}

	// Determine if write would overflow the capacity of the buffer and if
	// so, throw.
	//   delta: Delta in size expressed in uint64 words.
	void check_write_overflow(uint64_t delta)
	{
		if (_write_offset + delta > _cap)
			throw_overflow("Requested write of ", "capacity", delta, _write_offset,
				       _cap);
	}
};
} // namespace janus
