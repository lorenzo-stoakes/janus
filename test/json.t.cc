#include "dynamic_buffer.hh"
#include "json.hh"

#include <cstdint>
#include <cstring>
#include <gtest/gtest.h>

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

// Test that the remove_outer_array() internal function correctly removes outer
// [] from JSON or if not present leaves the JSON as-is.
TEST(json_test, remove_outer_array)
{
	char invalid_buf[] = "123";

	uint64_t size = 0;
	// Anything less than size 3 should fail.
	EXPECT_THROW(janus::internal::remove_outer_array(invalid_buf, size), std::runtime_error);
	size = 1;
	EXPECT_THROW(janus::internal::remove_outer_array(invalid_buf, size), std::runtime_error);
	size = 2;
	EXPECT_THROW(janus::internal::remove_outer_array(invalid_buf, size), std::runtime_error);

	// Now check a normal case.
	char buf1[] = R"([{"x":3}])";
	uint64_t buf1_orig_size = sizeof(buf1) - 1;
	size = buf1_orig_size;
	char* ret1 = janus::internal::remove_outer_array(buf1, size);
	EXPECT_EQ(size, buf1_orig_size - 2);
	for (uint64_t i = 1; i < buf1_orig_size - 1; i++) {
		EXPECT_EQ(buf1[i], ret1[i - 1]);
	}

	// Now check a normal case with trailing whitespace.
	char buf2[] = R"([{"x":3}]      )";
	uint64_t buf2_orig_size = sizeof(buf2) - 1;
	size = buf2_orig_size;
	char* ret2 = janus::internal::remove_outer_array(buf2, size);
	// Expect [] and trailing whitespace to be removed.
	EXPECT_EQ(size, buf2_orig_size - 2 - 6);
	for (uint64_t i = 1; i < 8; i++) {
		EXPECT_EQ(buf2[i], ret2[i - 1]);
	}

	// Input JSON without an outer array should remain the same.
	char buf3[] = R"({"x":3, "y": 1.23456, "z": ["ohello!"])";
	uint64_t buf3_orig_size = sizeof(buf3) - 1;
	size = buf3_orig_size;
	char* ret3 = janus::internal::remove_outer_array(buf3, size);
	EXPECT_EQ(size, buf3_orig_size);
	EXPECT_EQ(std::strcmp(buf3, ret3), 0);

	// Invalid JSON without closing brace should raise an error here.
	char buf4[] = R"([{"x":3})";
	size = sizeof(buf4) - 1;
	EXPECT_THROW(janus::internal::remove_outer_array(buf4, size), std::runtime_error);
}
} // namespace
