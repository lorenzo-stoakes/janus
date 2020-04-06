#include "janus.hh"

#include <stdexcept>
#include <string>

namespace janus::betfair
{
auto parse_meta_json(const char* filename, char* str, uint64_t size, dynamic_buffer& dyn_buf)
	-> uint64_t
{
	// We need at least 3 characters in order to handle legacy metadata JSON files.
	if (size < 3) {
		throw std::runtime_error(std::string("JSON string '") + str + "' is too small.");
	}

	sajson::document doc = internal::parse_json(filename, str, size);

	return 0;
}
} // namespace janus::betfair
