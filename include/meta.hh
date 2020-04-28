#pragma once

#include <cstdint>

#include "dynamic_buffer.hh"
#include "sajson.hh"

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
