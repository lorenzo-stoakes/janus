#pragma once

#include <array>
#include <cstdint>
#include <stdexcept>
#include <string>

namespace janus
{
// Represents an array of T elements up to Cap size, taking up sizeof(T) * Cap + sizeof(uint64_t)
// space. The type allows us to avoid allocations while maintaining a dynamic container.
template<typename T, uint64_t Cap>
class dynamic_array
{
public:
	dynamic_array() : _size{0}, _raw_buf{0} {}
	// We need to manually destruct emplaced values.
	~dynamic_array()
	{
		destroy();
	}

	// We cannot move the array as the underlying data is part of
	// the object itself. We do not want to copy for efficiency.
	dynamic_array(const dynamic_array&) = delete;
	dynamic_array(dynamic_array&&) = delete;
	auto operator=(const dynamic_array&) -> dynamic_array& = delete;
	auto operator=(dynamic_array&& that) -> dynamic_array& = delete;

	// Retrieve MUTABLE element at specific index UNCHECKED.
	auto operator[](uint64_t i) -> T&
	{
		return *get_ptr(i);
	}

	// Retrieve element at specified index but check that it is within
	// bounds first.
	auto checked(uint64_t i) -> T&
	{
		if (i >= _size)
			throw std::runtime_error("Attempting to access index " + std::to_string(i) +
						 " but array is of size " + std::to_string(_size));

		return *get_ptr(i);
	}

	// Obtain current size of array.
	auto size() const -> uint64_t
	{
		return _size;
	}

	// Emplace a value at the back of the array.
	// Returns a reference to the newly emplaced value.
	template<typename... Args>
	auto emplace_back(Args&&... args) -> T&
	{
		return *new (&_raw_buf[sizeof(T) * _size++]) T(std::forward<Args>(args)...);
	}

	// Truncate the array to size 0, destructing existing elements.
	void clear()
	{
		destroy();
	}

	// Truncate the array to size 0, do NOT destruct the elements.
	void truncate()
	{
		_size = 0;
	}

private:
	// Get a pointer to the specified element in the array.
	auto get_ptr(uint64_t i) -> T*
	{
		return reinterpret_cast<T*>(&_raw_buf[sizeof(T) * i]);
	}

	// Destruct all elements.
	void destroy()
	{
		// The elements were created using placement new, this means
		// their dtors will NOT automatically execute so we have to
		// invoke them manually.
		for (uint64_t i = 0; i < _size; i++) {
			get_ptr(i)->~T();
		}
		_size = 0;
	}

	uint64_t _size;
	uint8_t _raw_buf[sizeof(T) * Cap]; // NOLINT: We don't want std::array semantics.
};
} // namespace janus
