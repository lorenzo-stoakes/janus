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
	ASSERT_EQ(ids.size(), 2);
	EXPECT_EQ(ids[0], 123);
	EXPECT_EQ(ids[1], 456);

	const auto market_ids = janus::get_meta_market_id_list(config);
	ASSERT_EQ(market_ids.size(), 2);
	EXPECT_EQ(market_ids[0], 170020946);
	EXPECT_EQ(market_ids[1], 170030493);

	janus::dynamic_buffer dyn_buf(10'000'000);

	const auto metas = janus::read_all_metadata(config, dyn_buf);
	ASSERT_EQ(metas.size(), 2);

	// We should have read everything that we wrote.
	EXPECT_EQ(dyn_buf.size(), dyn_buf.read_offset());

	// No need to check every detail, this is covered in meta view tests
	// already.
	EXPECT_EQ(metas[0].market_id(), 170020946);
	EXPECT_EQ(metas[0].runners().size(), 13);
	EXPECT_EQ(metas[1].market_id(), 170030493);
	EXPECT_EQ(metas[1].runners().size(), 2);

	// Now test the single market read function works correctly
	// individually.
	auto view = janus::read_metadata(config, dyn_buf, 170030493);

	// Again, we should have read everything that we wrote.
	EXPECT_EQ(dyn_buf.size(), dyn_buf.read_offset());

	EXPECT_EQ(view.market_id(), 170030493);
	EXPECT_EQ(view.runners().size(), 2);

	// Now check we can read update data.
	EXPECT_EQ(janus::read_market_updates(config, dyn_buf, 170358161), 1843);

	// check we can handle snappy-compressed archived stream updates too.
	EXPECT_EQ(janus::read_market_updates(config, dyn_buf, 168216153), 109648);
}
} // namespace
