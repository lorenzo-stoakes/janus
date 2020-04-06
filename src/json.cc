#include "janus.hh"

#include <stdexcept>
#include <string>

namespace janus
{
namespace internal
{
auto remove_outer_array(char* str, uint64_t& size) -> char*
{
	// We need at least 3 characters for the code below to functinon correctly.
	if (size < 3)
		throw std::runtime_error(std::string("JSON string '") + str + "' is too small.");

	if (str[0] != '[')
		return str;

	// Some older metadata files contain the data as a single element in an
	// array. Because the root element is const we can't just parse and get
	// the 0 element, so move our pointer and reduce the size until we have
	// eliminated the array.
	// We assume there is no leading whitespace here, if there is then just
	// live with the error.

	str = &str[1];
	size--;
	while (size > 0 && str[size - 1] != ']')
		size--;
	if (size == 0)
		throw std::runtime_error(std::string("Invalid JSON '") + str +
					 "' - cannot terminate [");
	// Remove final ']'.
	size--;

	return str;
}
} // namespace internal

namespace betfair
{
auto parse_meta_json(const char* filename, char* str, uint64_t size, dynamic_buffer& dyn_buf)
	-> uint64_t
{
	str = internal::remove_outer_array(str, size);
	sajson::document doc = internal::parse_json(filename, str, size);

	return 0;
}
} // namespace betfair
} // namespace janus
