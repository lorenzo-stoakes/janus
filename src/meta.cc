#include "janus.hh"

#include <cstdlib>
#include <ctime>
#include <iomanip>
#include <sstream>
#include <stdexcept>
#include <string>

#include <iostream>

namespace janus
{
runner_view::runner_view(dynamic_buffer& dyn_buf)
{
	_id = dyn_buf.read_uint64();
	_sort_priority = dyn_buf.read_uint64();
	_name = dyn_buf.read_string();
	_jockey_name = dyn_buf.read_string();
	_trainer_name = dyn_buf.read_string();
}

meta_view::meta_view(dynamic_buffer& dyn_buf) : _header{&dyn_buf.read<meta_header>()}
{
	_name = dyn_buf.read_string();
	_event_type_name = dyn_buf.read_string();
	_event_name = dyn_buf.read_string();
	_event_country_code = dyn_buf.read_string();
	_event_timezone = dyn_buf.read_string();
	_market_type_name = dyn_buf.read_string();
	_venue_name = dyn_buf.read_string();
	_competition_name = dyn_buf.read_string();

	for (uint64_t i = 0; i < _header->num_runners; i++) {
		_runners.emplace_back(dyn_buf);
	}
}

namespace betfair
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

auto extract_meta_header(const sajson::value& node, dynamic_buffer& dyn_buf) -> uint64_t
{
	uint64_t prev_size = dyn_buf.size();

	// Each value is required, if one is not found, an error will be raised.
	meta_header header = {0};
	sajson::value market_id_node = node.get_value_of_key(sajson::literal("marketId"));
	header.market_id = janus::internal::parse_market_id(market_id_node.as_cstring(),
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
	if (competition_node.get_type() != sajson::TYPE_NULL) {
		sajson::value competition_id_node =
			competition_node.get_value_of_key(sajson::literal("id"));
		if (competition_id_node.get_string_length() > 0)
			header.competition_id = std::atol(competition_id_node.as_cstring());
	}

	sajson::value runners_node = node.get_value_of_key(sajson::literal("runners"));
	header.num_runners = runners_node.get_length();

	dyn_buf.add(header);
	return dyn_buf.size() - prev_size;
}

auto extract_meta_static_strings(const sajson::value& node, dynamic_buffer& dyn_buf) -> uint64_t
{
	uint64_t prev_size = dyn_buf.size();

	dyn_buf.add_string(node.get_value_of_key(sajson::literal("marketName")));

	sajson::value event_type_node = node.get_value_of_key(sajson::literal("eventType"));
	dyn_buf.add_string(event_type_node.get_value_of_key(sajson::literal("name")));

	sajson::value event_node = node.get_value_of_key(sajson::literal("event"));
	dyn_buf.add_string(event_node.get_value_of_key(sajson::literal("name")));
	dyn_buf.add_string(event_node.get_value_of_key(sajson::literal("countryCode")));
	dyn_buf.add_string(event_node.get_value_of_key(sajson::literal("timezone")));

	sajson::value description_node = node.get_value_of_key(sajson::literal("description"));
	if (description_node.get_type() == sajson::TYPE_NULL)
		dyn_buf.add_string(nullptr, 0);
	else
		dyn_buf.add_string(
			description_node.get_value_of_key(sajson::literal("marketType")));

	sajson::value venue_node = event_node.get_value_of_key(sajson::literal("venue"));
	dyn_buf.add_string(venue_node);

	sajson::value competition_node = node.get_value_of_key(sajson::literal("competition"));
	if (competition_node.get_type() == sajson::TYPE_NULL) {
		dyn_buf.add_string(nullptr, 0);
	} else {
		dyn_buf.add_string(competition_node.get_value_of_key(sajson::literal("name")));
	}

	return dyn_buf.size() - prev_size;
}

auto extract_meta_runners(const sajson::value& node, dynamic_buffer& dyn_buf) -> uint64_t
{
	uint64_t prev_size = dyn_buf.size();

	uint64_t count = node.get_length();
	for (uint64_t i = 0; i < count; i++) {
		sajson::value runner_node = node.get_array_element(i);

		uint64_t id = runner_node.get_value_of_key(sajson::literal("selectionId"))
				      .get_integer_value();
		uint64_t sort_priority =
			runner_node.get_value_of_key(sajson::literal("sortPriority"))
				.get_integer_value();
		sajson::value name_node =
			runner_node.get_value_of_key(sajson::literal("runnerName"));

		dyn_buf.add_uint64(id);
		dyn_buf.add_uint64(sort_priority);
		dyn_buf.add_string(name_node);

		sajson::value metadata_node =
			runner_node.get_value_of_key(sajson::literal("metadata"));
		if (metadata_node.get_type() == sajson::TYPE_NULL) {
			// Add empty jockey, trainer names.
			dyn_buf.add_string("", 0);
			dyn_buf.add_string("", 0);
			continue;
		}

		sajson::value jockey_name_node =
			metadata_node.get_value_of_key(sajson::literal("JOCKEY_NAME"));
		dyn_buf.add_string(jockey_name_node);

		sajson::value trainer_name_node =
			metadata_node.get_value_of_key(sajson::literal("TRAINER_NAME"));
		dyn_buf.add_string(trainer_name_node);
	}

	return dyn_buf.size() - prev_size;
}
} // namespace internal

auto parse_meta_json(const sajson::value& root, dynamic_buffer& dyn_buf) -> uint64_t
{
	uint64_t count = internal::extract_meta_header(root, dyn_buf);
	count += internal::extract_meta_static_strings(root, dyn_buf);
	count += internal::extract_meta_runners(root.get_value_of_key(sajson::literal("runners")),
						dyn_buf);

	return count;
}

auto parse_meta_json(const char* filename, char* str, uint64_t size, dynamic_buffer& dyn_buf)
	-> uint64_t
{
	str = internal::remove_outer_array(str, size);
	sajson::document doc = janus::internal::parse_json(filename, str, size);
	const sajson::value& root = doc.get_root();

	return parse_meta_json(root, dyn_buf);
}
} // namespace betfair

auto meta_view::describe(bool show_date) const -> std::string
{
	uint64_t year, month, day, ms;
	uint64_t hour, minute, second;
	local_unpack_epoch_ms(market_start_timestamp(), year, month, day, hour, minute, second, ms);

	std::ostringstream oss;
	oss << std::setfill('0');

	if (show_date)
		oss << year << "-" << std::setw(2) << month << "-" << std::setw(2) << day << " ";

	oss << std::setw(2) << hour << ":" << std::setw(2) << minute << ":" << std::setw(2)
	    << second << " / ";

	oss << event_country_code() << " / ";

	// If horse racing then output venue as neater.
	if (event_type_id() == 7) // NOLINT: Not magical, horses!
		oss << venue_name();
	else
		oss << event_name();

	oss << " / " << name();
	return oss.str();
}
} // namespace janus
