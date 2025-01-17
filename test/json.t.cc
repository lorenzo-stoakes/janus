#include "janus.hh"

#include <cstdint>
#include <gtest/gtest.h>
#include <string>

namespace
{
// Test that the internal function parse_json() correctly returns an
// sajson::document or throws an exception on invalid input.
TEST(json_test, parse_json)
{
	// An empty input should throw.
	char buf1[] = "";
	EXPECT_THROW(janus::internal::parse_json("", buf1, sizeof(buf1) - 1),
		     janus::json_parse_error);

	// Unclosed brackets should too.
	char buf2[] = "[[({";
	EXPECT_THROW(janus::internal::parse_json("", buf2, sizeof(buf2) - 1),
		     janus::json_parse_error);

	// We'll leave further invalid input checks to the sajson-specific tests.

	// Check a simple input.
	char buf3[] = "[{\"x\":3}]";
	sajson::document doc = janus::internal::parse_json("", buf3, sizeof(buf3) - 1);
	const sajson::value& root = doc.get_root();

	EXPECT_EQ(root.get_type(), sajson::TYPE_ARRAY);

	EXPECT_EQ(root.get_array_element(0)
			  .get_value_of_key(sajson::literal("x"))
			  .get_integer_value(),
		  3);
}

// Test that errors generated with a line offset specified correctly update the
// error message.
TEST(json_test, parse_json_line_offset_error)
{
	char buf[] = "[[({";

	// First without line offset.
	try {
		janus::internal::parse_json("foo", buf, sizeof(buf) - 1);
	} catch (janus::json_parse_error& e) {
		EXPECT_STREQ(e.what(), "JSON parse error: foo:1:3: expected value");
	}

	// And now with a 3 line offset.
	try {
		janus::internal::parse_json("foo", buf, sizeof(buf) - 1, 3);
	} catch (janus::json_parse_error& e) {
		EXPECT_STREQ(e.what(), "JSON parse error: foo:4:3: expected value");
	}
}
} // namespace
