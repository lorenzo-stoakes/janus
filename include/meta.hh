#pragma once

#include "dynamic_buffer.hh"
#include "sajson.hh"

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace janus
{
struct meta_header
{
	uint64_t market_id;
	uint64_t event_type_id;
	uint64_t event_id;
	uint64_t competition_id;
	uint64_t market_start_timestamp;
	uint64_t num_runners;
};

class runner_view
{
public:
	explicit runner_view(dynamic_buffer& dyn_buf);

	auto id() const -> uint64_t
	{
		return _id;
	}

	auto sort_priority() const -> uint64_t
	{
		return _sort_priority;
	}

	auto name() const -> std::string_view
	{
		return _name;
	}

	auto jockey_name() const -> std::string_view
	{
		return _jockey_name;
	}

	auto trainer_name() const -> std::string_view
	{
		return _trainer_name;
	}

private:
	uint64_t _id;
	uint64_t _sort_priority;
	std::string_view _name;
	std::string_view _jockey_name;
	std::string_view _trainer_name;
};

class meta_view
{
public:
	explicit meta_view(dynamic_buffer& dyn_buf);

	// Provide a short description of the market.
	auto describe() -> std::string;

	auto market_id() const -> uint64_t
	{
		return _header->market_id;
	}

	auto event_type_id() const -> uint64_t
	{
		return _header->event_type_id;
	}

	auto event_id() const -> uint64_t
	{
		return _header->event_id;
	}

	auto competition_id() const -> uint64_t
	{
		return _header->competition_id;
	}

	auto market_start_timestamp() const -> uint64_t
	{
		return _header->market_start_timestamp;
	}

	auto name() const -> std::string_view
	{
		return _name;
	}

	auto event_type_name() const -> std::string_view
	{
		return _event_type_name;
	}

	auto event_name() const -> std::string_view
	{
		return _event_name;
	}

	auto event_country_code() const -> std::string_view
	{
		return _event_country_code;
	}

	auto event_timezone() const -> std::string_view
	{
		return _event_timezone;
	}

	auto market_type_name() const -> std::string_view
	{
		return _market_type_name;
	}

	auto venue_name() const -> std::string_view
	{
		return _venue_name;
	}

	auto competition_name() const -> std::string_view
	{
		return _competition_name;
	}

	auto runners() const -> const std::vector<runner_view>&
	{
		return _runners;
	}

private:
	const meta_header* _header;
	std::string_view _name;
	std::string_view _event_type_name;
	std::string_view _event_name;
	std::string_view _event_country_code;
	std::string_view _event_timezone;
	std::string_view _market_type_name;
	std::string_view _venue_name;
	std::string_view _competition_name;

	std::vector<runner_view> _runners;
};

namespace betfair
{
namespace internal
{
// Remove outer array from a JSON string buffer, if present. This is used to
// work around legacy metadata files which placed data as a single object within
// an array.
//        str: Char buffer containing JSON to parse. WILL BE MUTATED.
//       size: Size of char buffer.
//    returns: The new size after the adjustment has been made.
auto remove_outer_array(char* str, uint64_t& size) -> char*;

// Extract the betfair metadata header from the metadata JSON object node and
// place in binary file format in the specified dynamic buffer.
//      node: The SAJSON node containing the betfair metadata object data.
//   dyn_buf: The dynamic buffer to output the data to in binary file format.
//   returns: The number of BYTES written into the dynamic buffer.
auto extract_meta_header(const sajson::value& node, dynamic_buffer& dyn_buf) -> uint64_t;

// Extract static betfair metadata strings from the metadata JSON object node
// and place in binary file format in the specified dynamic buffer.
//      node: The SAJSON node containing the betfair metadata object data.
//   dyn_buf: The dynamic buffer to output the data to in binary file format.
//   returns: The number of BYTES written into the dynamic buffer.
auto extract_meta_static_strings(const sajson::value& node, dynamic_buffer& dyn_buf) -> uint64_t;

// Extract static betfair runner metadata from the metadata JSON runner array
// node.
//      node: The SAJSON node containing the betfair metadata runner array.
//   dyn_buf: The dynamic buffer to output the data to in binary file format.
//   returns: The number of BYTES written into the dynamic buffer.
auto extract_meta_runners(const sajson::value& node, dynamic_buffer& dyn_buf) -> uint64_t;
} // namespace internal

// Parse betfair market metadata JSON already parsed intl an AST by sajson and
// append data in the binary file format to the specified dynamic buffer.
//      root: Root sajson node to parse.
//   dyn_buf: Dynamic buffer to OUTPUT data to in binary file format.
//   returns: Number of BYTES appended to dynamic buffer.
auto parse_meta_json(const sajson::value& root, dynamic_buffer& dyn_buf) -> uint64_t;

// Parse betfair market metadata JSON and append data in the binary file format
// to the specified dynamic buffer. Throws if there is an issue with input JSON.
//   filename: Filename to refer to in error messages.
//        str: Char buffer containing JSON to parse. WILL BE MUTATED.
//       size: Size of char buffer.
//    dyn_buf: Dynamic buffer to OUTPUT data to in binary file format.
//    returns: Number of BYTES appended to dynamic buffer.
auto parse_meta_json(const char* filename, char* str, uint64_t size, dynamic_buffer& dyn_buf)
	-> uint64_t;
} // namespace betfair
} // namespace janus
