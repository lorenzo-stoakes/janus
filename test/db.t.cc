#include "janus.hh"

#include <gtest/gtest.h>

namespace
{
// Test that DB fundamentals work correctly.
TEST(db_test, basic)
{
	janus::config config = {
		.json_data_root = "../test/test-json",
	};

	const auto ids = janus::get_json_file_id_list(config);
	EXPECT_EQ(ids[0], 123);
	EXPECT_EQ(ids[1], 456);
}
} // namespace
