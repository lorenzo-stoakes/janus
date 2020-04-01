#pragma once

#include <cstdint>
#include <memory>
#include <stdexcept>
#include <string>

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
		  _size{0},
		  _buf{std::make_unique<uint64_t[]>(_cap)}
	{
	}

	// Make the buffer moveable.
	dynamic_buffer(dynamic_buffer&& that)
		: _cap{that._cap}, _size{that._size}, _buf{std::move(that._buf)}
	{
		that._cap = 0;
		that._buf = 0;
	}

	// The default dtor is fine, our underlying buffer is wrapped by a unique_ptr.
	~dynamic_buffer() = default;

	// Delete the copy ctor + assignment operator we want to discourage copying.
	dynamic_buffer(const dynamic_buffer&) = delete;
	dynamic_buffer& operator=(const dynamic_buffer&) = delete;
	// Discourage move assignment as well, we don't want to be overwriting
	// a buffer.
	dynamic_buffer& operator=(dynamic_buffer&&) = delete;

	// Overall maximum capacity of the buffer in bytes. If this is exceeded, we
	// throw.
	auto cap() -> uint64_t const
	{
		// We store this value as a multiple of uint64_t words
		// internally.
		return _cap * sizeof(uint64_t);
	}

	// Current used bytes, also represents the offset at which data will be
	// written. size() <= cap() is an invariant.
	auto size() -> uint64_t const
	{
		// We store this value as a multiple of uint64_t words
		// internally.
		return _size * sizeof(uint64_t);
	}

	// Return raw underlying buffer.
	auto data() -> const uint64_t* const
	{
		return _buf.get();
	}

	// Add a uint64 value to the buffer.
	void add_uint64(uint64_t n)
	{
		check_overflow(1);
		_buf[_size++] = n;
	}

	// Clear the buffer but maintain the capacity.
	void reset()
	{
		_size = 0;
	}

private:
	// Capacity of buffer in uint64 words (i.e. cap_bytes = 8 * _cap).
	uint64_t _cap;
	// Size of buffer in uint64 words (i.e. size_bytes = 8 * _size).
	uint64_t _size;
	// Store our data as an array of uint64_t so we're (64-bit) word aligned
	// by default.
	std::unique_ptr<uint64_t[]> _buf;

	// Align input value to 64-bits, i.e. 8 bytes.
	static constexpr auto align64(uint64_t size) -> uint64_t
	{
		// Anything not aligned will increase to above the next 8-byte
		// aligned value, then by clearing the trailing bits we obtain
		// that value. Aligned values remain the same.
		return (size + 7) & ~7ULL;
	}

	// Determine if we would overflow the capacity of the buffer and if so,
	// throw.
	//   delta: Delta in size expressed in uint64 words.
	void check_overflow(uint64_t delta)
	{
		// We are OK with some string allocations on the exceptional
		// code path.
		if (_cap < _size + delta) {
			uint64_t delta_bytes = delta * sizeof(uint64_t);
			uint64_t size_bytes = _size * sizeof(uint64_t);
			uint64_t cap_bytes = _cap * sizeof(uint64_t);

			throw std::runtime_error(
				"Requested allocation of " + std::to_string(delta_bytes) +
				" + size of " + std::to_string(size_bytes) +
				" bytes exceeds capacity of " + std::to_string(cap_bytes));
		}
	}
};
} // namespace janus
