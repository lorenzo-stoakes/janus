#pragma once

#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <stdexcept>
#include <string>

namespace janus
{
// Helper class for generating HTTP.
class http_request
{
public:
	http_request(char* buf, uint64_t cap)
		: _buf{buf}, _size{0}, _cap{cap}, _complete{false}, _op_set{false}
	{
	}

	// Retrieve underlying buffer.
	auto buf() const -> char*
	{
		return _buf;
	}

	// Retrieve buffer size.
	auto size() const -> uint64_t
	{
		return _size;
	}

	// Retrieve buffer capacity.
	auto cap() const -> uint64_t
	{
		return _cap;
	}

	// Is this HTTP parse/generation complete?
	auto complete() const -> bool
	{
		return _complete;
	}

	// Generate generic request, returning the buffer for convenience.
	auto op(const char* op, const char* host, const char* path = "/") -> char*;

	// Generate GET request, returning the buffer for convenience.
	auto get(const char* host, const char* path = "/") -> char*
	{
		return op("GET", host, path);
	}

	// Generate POST request, returning the buffer for convenience.
	auto post(const char* host, const char* path = "/") -> char*
	{
		return op("POST", host, path);
	}

	// Append an HTTP header to the buffer, returning the buffer for
	// convenience.
	auto add_header(const char* key, const char* value) -> char*;

	// Terminate HTTP request, adding a newline and null terminator. This is
	// where no data is provided, e.g. for a GET request.
	auto terminate() -> char*;

	// Add data to the buffer. This sets Content-Length, correct newline
	// prefix, appends data, adds null terminator, marks request complete
	// and returns the buffer for convenience.
	auto add_data(const char* buf, uint32_t size) -> char*;

private:
	static constexpr uint64_t NUMBER_BUFFER_SIZE = 20;

	char* _buf;
	uint64_t _size;
	uint64_t _cap;
	bool _complete;
	bool _op_set;

	// Determine whether HTTP operation is complete - if so, throw error.
	void check_incomplete();

	// Assert that the operation has not been set.
	void check_op_unset();

	// Assert that the operation has been set.
	void check_op_set();

	// Check if we are below capacity (therefore have space to perform the
	// subsequent operation).
	void check_size();

	// Append a string to the buffer, WITHOUT null terminator.
	void append(const char* str);

	// Append a string to the buffer of specified size.
	void append(const char* str, uint64_t size);

	// Append null terminator to end of buffer.
	void append_null_terminator();

	// Append newline to buffer, by convention this is the windows newline
	// pattern.
	void append_newline();
};
} // namespace janus
