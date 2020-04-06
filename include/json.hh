#pragma once

#include "dynamic_buffer.hh"
#include "error.hh"
#include "sajson.hh"

#include <cstdint>

namespace janus
{
namespace internal
{
// Parse JSON into an sajson document file and check that it contains no errors.
//   filename: Filename to refer to in error messages.
//        str: Char buffer containing JSON to parse. WILL BE MUTATED.
//       size: Size of char buffer.
//    returns: sajson document object.
static inline auto parse_json(const char* filename, char* str, uint64_t size) -> sajson::document
{
	sajson::document doc =
		sajson::parse(sajson::single_allocation(), sajson::mutable_string_view(size, str));

	if (!doc.is_valid()) {
		int line = doc.get_error_line();
		int col = doc.get_error_column();
		const char* error = doc.get_error_message_as_cstring();

		throw json_parse_error(filename, line, col, error);
	}

	// We rely on RVO for performance.
	return doc;
}
} // namespace internal

namespace betfair
{
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
