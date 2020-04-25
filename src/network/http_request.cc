#include "janus.hh"

#include <stdexcept>

namespace janus
{
auto http_request::op(const char* op, const char* host, const char* path) -> char*
{
	check_incomplete();
	check_op_unset();

	append(op);
	append(" ");
	append(path);
	append(" HTTP/1.0");
	append_newline();

	_op_set = true;

	// Add host header.
	return add_header("Host", host);
}

auto http_request::add_header(const char* key, const char* value) -> char*
{
	check_incomplete();
	check_op_set();

	append(key);
	append(": ");
	append(value);
	append_newline();

	return _buf;
}

auto http_request::terminate() -> char*
{
	check_incomplete();
	check_op_set();

	if (_size == 0)
		throw std::runtime_error("Terminating an empty buffer?");

	if (_buf[_size - 1] != '\n')
		throw std::runtime_error("Terminating but no newline in last char?");

	append_newline();
	append_null_terminator();
	_complete = true;

	return _buf;
}

auto http_request::add_data(const char* buf, uint32_t size) -> char*
{
	check_incomplete();
	check_op_set();

	char num_buf[NUMBER_BUFFER_SIZE];
	// We (pretty safely) assume this will not be larger than our
	// buffer. If it is then, damn that's a lot of data!
	snprintf(num_buf, NUMBER_BUFFER_SIZE, "%u", size);

	add_header("Content-Length", num_buf);
	// Add additional newline to indicate data is beginning.
	append_newline();

	append(buf);
	append_null_terminator();
	_complete = true;

	return _buf;
}

void http_request::check_incomplete()
{
	if (_complete)
		throw std::runtime_error(
			"Trying to perform HTTP request on completed HTTP operation");
}

void http_request::check_op_unset()
{
	if (_op_set)
		throw std::runtime_error("Trying to add header when one already exists");
}

void http_request::check_op_set()
{
	if (!_op_set)
		throw std::runtime_error("Trying to perform operation without header");
}

void http_request::check_size()
{
	if (_size == _cap)
		throw std::runtime_error("Cannot append, buffer at maximum capacity of " +
					 std::to_string(_cap));
}

void http_request::append(const char* str)
{
	while (*str != '\0') {
		check_size();
		_buf[_size++] = *str++;
	}
}

void http_request::append_null_terminator()
{
	check_size();
	_buf[_size++] = '\0';
}

void http_request::append_newline()
{
	append("\r\n");
}
} // namespace janus
