#include "janus.hh"

#include <cstdint>
#include <cstring>
#include <stdexcept>

namespace janus
{
// Find needle in haystack and return offset past the found string.
template<uint64_t sz>
static auto find_str(const char* haystack, const char (&needle)[sz], int offset = 0) -> int
{
	const char* ptr = ::strstr(&haystack[offset], needle);
	if (ptr == nullptr)
		return -1;

	ptrdiff_t ret = ptr - &haystack[offset];
	return static_cast<int>(ret) + sz - 1;
}

auto parse_http_response(const char* buf, uint64_t size, int& response_code, uint64_t& offset)
	-> uint64_t
{
	int start = find_str(buf, " ");
	if (start == -1)
		throw std::runtime_error("Invalid HTTP, cannot find start of response code");

	response_code = ::atoi(&buf[start]);

	start = find_str(buf, "Content-Length: ");
	if (start == -1) {
		// If we can't find content-length, assume no data.
		offset = 0;
		return 0;
	}
	uint64_t length = ::atoll(&buf[start]);

	start = find_str(buf, "\r\n\r\n");
	if (start == -1)
		throw std::runtime_error("Invalid HTTP, cannot find data");
	offset = start;

	uint64_t data_length = size - offset + 1;

	uint64_t remaining_bytes = data_length > length ? 0 : length - data_length;
	return remaining_bytes;
}
} // namespace janus
