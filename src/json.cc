#include "janus.hh"

#include <cstdlib>
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

auto betfair_extract_meta_header(const sajson::value& node, dynamic_buffer& dyn_buf) -> uint64_t
{
	uint64_t prev_size = dyn_buf.size();

	// Each value is required, if one is not found, an error will be raised.
	meta_header header = {0};
	sajson::value market_id_node = node.get_value_of_key(sajson::literal("marketId"));
	header.market_id = internal::parse_market_id(market_id_node.as_cstring(),
						     market_id_node.get_string_length());
	sajson::value start_time_node = node.get_value_of_key(sajson::literal("marketStartTime"));
	header.market_start_timestamp = janus::parse_iso8601(start_time_node.as_cstring(),
							     start_time_node.get_string_length());

	sajson::value event_type_node = node.get_value_of_key(sajson::literal("eventType"));
	// ID is stored as a string for some reason.
	const char* event_type_id_str =
		event_type_node.get_value_of_key(sajson::literal("id")).as_cstring();
	header.event_type_id = std::atol(event_type_id_str);

	sajson::value event_node = node.get_value_of_key(sajson::literal("event"));
	const char* event_id_str = event_node.get_value_of_key(sajson::literal("id")).as_cstring();
	header.event_id = std::atol(event_id_str);

	// Competition is optional (not in life).
	sajson::value competition_node = node.get_value_of_key(sajson::literal("competition"));
	sajson::value competition_id_node =
		competition_node.get_value_of_key(sajson::literal("id"));
	if (competition_id_node.get_string_length() > 0)
		header.competition_id = std::atol(competition_id_node.as_cstring());

	sajson::value runners_node = node.get_value_of_key(sajson::literal("runners"));
	header.num_runners = runners_node.get_length();

	dyn_buf.add(header);
	return dyn_buf.size() - prev_size;
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
