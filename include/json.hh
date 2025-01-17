#pragma once

#include "dynamic_buffer.hh"
#include "error.hh"
#include "sajson.hh"

#include <cstdint>

namespace janus::internal
{
// Parse JSON into an sajson document file and check that it contains no errors.
//    filename: Filename to refer to in error messages.
//         str: Char buffer containing JSON to parse. WILL BE MUTATED.
//        size: Size of char buffer.
// line_offset: Number of lines to offset reported line number by.
//     returns: sajson document object.
static inline auto parse_json(const char* filename, char* str, uint64_t size,
			      uint64_t line_offset = 0) -> sajson::document
{
	sajson::document doc =
		sajson::parse(sajson::single_allocation(), sajson::mutable_string_view(size, str));

	if (!doc.is_valid()) {
		uint64_t line = doc.get_error_line() + line_offset;
		uint64_t col = doc.get_error_column();
		const char* error = doc.get_error_message_as_cstring();

		throw json_parse_error(filename, line, col, error);
	}

	// We rely on RVO for performance.
	return doc;
}
} // namespace janus::internal
