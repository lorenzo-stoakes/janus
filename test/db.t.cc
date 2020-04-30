#include "janus.hh"

#include <gtest/gtest.h>

namespace
{
// Test that DB fundamentals work correctly.
TEST(db_test, basic)
{
	janus::config config = {
		.json_data_root = "../test/test-json",
		.binary_data_root = "../test/test-binary",
	};

	const auto ids = janus::get_json_file_id_list(config);
	EXPECT_EQ(ids.size(), 2);
	EXPECT_EQ(ids[0], 123);
	EXPECT_EQ(ids[1], 456);

	const auto market_ids = janus::get_meta_market_id_list(config);
	EXPECT_EQ(market_ids.size(), 2);
	EXPECT_EQ(market_ids[0], 123456);
	EXPECT_EQ(market_ids[1], 999888);
}
} // namespace
