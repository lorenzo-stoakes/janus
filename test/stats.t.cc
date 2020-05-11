#include "janus.hh"

#include <cmath>
#include <cstdint>
#include <gtest/gtest.h>

namespace
{
// Quick and dirty function to round a number to 2 decimal places.
static double round_2dp(double n)
{
	uint64_t nx100 = ::llround(n * 100.);
	return static_cast<double>(nx100) / 100;
}

// Test that an empty set of updates results in zero stats.
TEST(stats_test, empty_updates)
{
	janus::dynamic_buffer dyn_buf(0);
	janus::stats stats1 = janus::generate_stats(nullptr, dyn_buf);
	EXPECT_EQ(dyn_buf.read_offset(), 0);
	EXPECT_EQ(stats1.flags, janus::stats_flags::DEFAULT);
	EXPECT_EQ(stats1.num_updates, 0);
	EXPECT_EQ(stats1.num_runners, 0);
	EXPECT_EQ(stats1.num_removals, 0);
	EXPECT_EQ(stats1.first_timestamp, 0);
	EXPECT_EQ(stats1.start_timestamp, 0);
	EXPECT_EQ(stats1.inplay_timestamp, 0);
	EXPECT_EQ(stats1.last_timestamp, 0);
	EXPECT_EQ(stats1.winner_runner_id, 0);
	for (uint64_t i = 0; i < janus::NUM_STATS_INTERVALS; i++) {
		EXPECT_EQ(stats1.pre_post_intervals[i].num_updates, 0);
		EXPECT_DOUBLE_EQ(stats1.pre_post_intervals[i].mean_update_interval_ms, 0);
		EXPECT_EQ(stats1.pre_post_intervals[i].worst_update_interval_ms, 0);

		EXPECT_EQ(stats1.pre_inplay_intervals[i].num_updates, 0);
		EXPECT_DOUBLE_EQ(stats1.pre_inplay_intervals[i].mean_update_interval_ms, 0);
		EXPECT_EQ(stats1.pre_inplay_intervals[i].worst_update_interval_ms, 0);
	}
	EXPECT_EQ(stats1.inplay_intervals.num_updates, 0);
	EXPECT_DOUBLE_EQ(stats1.inplay_intervals.mean_update_interval_ms, 0);
	EXPECT_EQ(stats1.inplay_intervals.worst_update_interval_ms, 0);
}

// Test that a single timestamp results in expected stats.
TEST(stats_test, single_timestamp)
{
	janus::dynamic_buffer dyn_buf(16);
	dyn_buf.add(janus::make_timestamp_update(1589178362508));
	janus::stats stats1 = janus::generate_stats(nullptr, dyn_buf);
	EXPECT_EQ(dyn_buf.read_offset(), 16);
	EXPECT_EQ(stats1.flags, janus::stats_flags::DEFAULT);
	EXPECT_EQ(stats1.num_updates, 1);
	EXPECT_EQ(stats1.num_runners, 0);
	EXPECT_EQ(stats1.num_removals, 0);
	EXPECT_EQ(stats1.first_timestamp, 1589178362508);
	EXPECT_EQ(stats1.start_timestamp, 0);
	EXPECT_EQ(stats1.inplay_timestamp, 0);
	EXPECT_EQ(stats1.last_timestamp, 1589178362508);
	EXPECT_EQ(stats1.winner_runner_id, 0);
	for (uint64_t i = 0; i < janus::NUM_STATS_INTERVALS; i++) {
		EXPECT_EQ(stats1.pre_post_intervals[i].num_updates, 0);
		EXPECT_DOUBLE_EQ(stats1.pre_post_intervals[i].mean_update_interval_ms, 0);
		EXPECT_EQ(stats1.pre_post_intervals[i].worst_update_interval_ms, 0);

		EXPECT_EQ(stats1.pre_inplay_intervals[i].num_updates, 0);
		EXPECT_DOUBLE_EQ(stats1.pre_inplay_intervals[i].mean_update_interval_ms, 0);
		EXPECT_EQ(stats1.pre_inplay_intervals[i].worst_update_interval_ms, 0);
	}
	EXPECT_EQ(stats1.inplay_intervals.num_updates, 0);
	EXPECT_DOUBLE_EQ(stats1.inplay_intervals.mean_update_interval_ms, 0);
	EXPECT_EQ(stats1.inplay_intervals.worst_update_interval_ms, 0);
}

// Test that a start and end timestamp result in expected stats.
TEST(stats_test, double_timestamp)
{
	janus::dynamic_buffer dyn_buf(32);
	dyn_buf.add(janus::make_timestamp_update(1589178362508));
	dyn_buf.add(janus::make_timestamp_update(1589178462508));

	janus::stats stats1 = janus::generate_stats(nullptr, dyn_buf);
	EXPECT_EQ(dyn_buf.read_offset(), 32);
	EXPECT_EQ(stats1.flags, janus::stats_flags::DEFAULT);
	EXPECT_EQ(stats1.num_updates, 2);
	EXPECT_EQ(stats1.num_runners, 0);
	EXPECT_EQ(stats1.num_removals, 0);
	EXPECT_EQ(stats1.first_timestamp, 1589178362508);
	EXPECT_EQ(stats1.start_timestamp, 0);
	EXPECT_EQ(stats1.inplay_timestamp, 0);
	EXPECT_EQ(stats1.last_timestamp, 1589178462508);
	EXPECT_EQ(stats1.winner_runner_id, 0);
	for (uint64_t i = 0; i < janus::NUM_STATS_INTERVALS; i++) {
		EXPECT_EQ(stats1.pre_post_intervals[i].num_updates, 0);
		EXPECT_DOUBLE_EQ(stats1.pre_post_intervals[i].mean_update_interval_ms, 0);
		EXPECT_EQ(stats1.pre_post_intervals[i].worst_update_interval_ms, 0);

		EXPECT_EQ(stats1.pre_inplay_intervals[i].num_updates, 0);
		EXPECT_DOUBLE_EQ(stats1.pre_inplay_intervals[i].mean_update_interval_ms, 0);
		EXPECT_EQ(stats1.pre_inplay_intervals[i].worst_update_interval_ms, 0);
	}
	EXPECT_EQ(stats1.inplay_intervals.num_updates, 0);
	EXPECT_DOUBLE_EQ(stats1.inplay_intervals.mean_update_interval_ms, 0);
	EXPECT_EQ(stats1.inplay_intervals.worst_update_interval_ms, 0);
}

// Test that we gather correct stats in the presence of a simple metadata view.
TEST(stats_test, with_simple_meta_view)
{
	janus::dynamic_buffer dyn_buf(32);
	dyn_buf.add(janus::make_timestamp_update(1589178362508));
	dyn_buf.add(janus::make_timestamp_update(1589178462508));

	// We extract number of runners and market start time from the metadata.
	janus::meta_header header = {
		.market_start_timestamp = 1234567,
	};
	janus::meta_view meta(&header);
	meta.runners_mutable().push_back(janus::runner_view());
	meta.runners_mutable().push_back(janus::runner_view());

	janus::stats stats1 = janus::generate_stats(&meta, dyn_buf);
	EXPECT_EQ(dyn_buf.read_offset(), 32);
	EXPECT_EQ(stats1.flags, janus::stats_flags::HAVE_METADATA | janus::stats_flags::PAST_POST);
	EXPECT_EQ(stats1.num_updates, 2);
	EXPECT_EQ(stats1.num_runners, 2);
	EXPECT_EQ(stats1.num_removals, 0);
	EXPECT_EQ(stats1.first_timestamp, 1589178362508);
	EXPECT_EQ(stats1.start_timestamp, 1234567);
	EXPECT_EQ(stats1.inplay_timestamp, 0);
	EXPECT_EQ(stats1.last_timestamp, 1589178462508);
	EXPECT_EQ(stats1.winner_runner_id, 0);
	for (uint64_t i = 0; i < janus::NUM_STATS_INTERVALS; i++) {
		EXPECT_EQ(stats1.pre_post_intervals[i].num_updates, 0);
		EXPECT_DOUBLE_EQ(stats1.pre_post_intervals[i].mean_update_interval_ms, 0);
		EXPECT_EQ(stats1.pre_post_intervals[i].worst_update_interval_ms, 0);

		EXPECT_EQ(stats1.pre_inplay_intervals[i].num_updates, 0);
		EXPECT_DOUBLE_EQ(stats1.pre_inplay_intervals[i].mean_update_interval_ms, 0);
		EXPECT_EQ(stats1.pre_inplay_intervals[i].worst_update_interval_ms, 0);
	}
	EXPECT_EQ(stats1.inplay_intervals.num_updates, 0);
	EXPECT_DOUBLE_EQ(stats1.inplay_intervals.mean_update_interval_ms, 0);
	EXPECT_EQ(stats1.inplay_intervals.worst_update_interval_ms, 0);

	// Now test where we're not past post.
	dyn_buf.reset_read();
	janus::meta_header header2 = {
		.market_start_timestamp = 1589178462508,
	};
	janus::meta_view meta2(&header2);
	janus::stats stats2 = janus::generate_stats(&meta2, dyn_buf);
	EXPECT_EQ(stats2.flags, janus::stats_flags::HAVE_METADATA);
	EXPECT_EQ(stats2.num_runners, 0);

	// And when we're _just_ past post.
	dyn_buf.reset_read();
	janus::meta_header header3 = {
		.market_start_timestamp = 1589178362508,
	};
	janus::meta_view meta3(&header3);
	janus::stats stats3 = janus::generate_stats(&meta3, dyn_buf);
	EXPECT_EQ(stats3.flags, janus::stats_flags::HAVE_METADATA | janus::stats_flags::PAST_POST);
	EXPECT_EQ(stats3.num_runners, 0);
}

// Test that runner removals are picked up correctly.
TEST(stats_test, runner_removals)
{
	janus::dynamic_buffer dyn_buf(144);
	dyn_buf.add(janus::make_timestamp_update(1589178362508));
	dyn_buf.add(janus::make_runner_id_update(123456));
	dyn_buf.add(janus::make_runner_removal_update(0.123));
	dyn_buf.add(janus::make_timestamp_update(1589178462508));
	dyn_buf.add(janus::make_runner_id_update(654321));
	dyn_buf.add(janus::make_runner_removal_update(0.123));
	dyn_buf.add(janus::make_timestamp_update(1589178462608));
	dyn_buf.add(janus::make_runner_id_update(654321));
	// Duplicate removal.
	dyn_buf.add(janus::make_runner_removal_update(0.123));

	// We extract number of runners and market start time from the metadata.
	janus::meta_header header = {
		.market_start_timestamp = 1234567,
	};
	janus::meta_view meta(&header);
	meta.runners_mutable().push_back(janus::runner_view());
	meta.runners_mutable().push_back(janus::runner_view());

	janus::stats stats1 = janus::generate_stats(&meta, dyn_buf);
	EXPECT_EQ(dyn_buf.read_offset(), 144);
	EXPECT_EQ(stats1.flags, janus::stats_flags::HAVE_METADATA | janus::stats_flags::PAST_POST);
	EXPECT_EQ(stats1.num_updates, 9);
	EXPECT_EQ(stats1.num_runners, 2);
	EXPECT_EQ(stats1.num_removals, 2);
	EXPECT_EQ(stats1.first_timestamp, 1589178362508);
	EXPECT_EQ(stats1.start_timestamp, 1234567);
	EXPECT_EQ(stats1.inplay_timestamp, 0);
	EXPECT_EQ(stats1.last_timestamp, 1589178462608);
	EXPECT_EQ(stats1.winner_runner_id, 0);
	for (uint64_t i = 0; i < janus::NUM_STATS_INTERVALS; i++) {
		EXPECT_EQ(stats1.pre_post_intervals[i].num_updates, 0);
		EXPECT_DOUBLE_EQ(stats1.pre_post_intervals[i].mean_update_interval_ms, 0);
		EXPECT_EQ(stats1.pre_post_intervals[i].worst_update_interval_ms, 0);

		EXPECT_EQ(stats1.pre_inplay_intervals[i].num_updates, 0);
		EXPECT_DOUBLE_EQ(stats1.pre_inplay_intervals[i].mean_update_interval_ms, 0);
		EXPECT_EQ(stats1.pre_inplay_intervals[i].worst_update_interval_ms, 0);
	}
	EXPECT_EQ(stats1.inplay_intervals.num_updates, 0);
	EXPECT_DOUBLE_EQ(stats1.inplay_intervals.mean_update_interval_ms, 0);
	EXPECT_EQ(stats1.inplay_intervals.worst_update_interval_ms, 0);
}

// Test that we record that we saw a market closed.
TEST(stats_test, was_closed)
{
	janus::dynamic_buffer dyn_buf(48);
	dyn_buf.add(janus::make_timestamp_update(1589178362508));
	dyn_buf.add(janus::make_timestamp_update(1589178462508));
	dyn_buf.add(janus::make_market_close_update());

	janus::stats stats1 = janus::generate_stats(nullptr, dyn_buf);
	EXPECT_EQ(dyn_buf.read_offset(), 48);
	EXPECT_EQ(stats1.flags, janus::stats_flags::WAS_CLOSED);
	EXPECT_EQ(stats1.num_updates, 3);
	EXPECT_EQ(stats1.num_runners, 0);
	EXPECT_EQ(stats1.num_removals, 0);
	EXPECT_EQ(stats1.first_timestamp, 1589178362508);
	EXPECT_EQ(stats1.start_timestamp, 0);
	EXPECT_EQ(stats1.inplay_timestamp, 0);
	EXPECT_EQ(stats1.last_timestamp, 1589178462508);
	EXPECT_EQ(stats1.winner_runner_id, 0);
	for (uint64_t i = 0; i < janus::NUM_STATS_INTERVALS; i++) {
		EXPECT_EQ(stats1.pre_post_intervals[i].num_updates, 0);
		EXPECT_DOUBLE_EQ(stats1.pre_post_intervals[i].mean_update_interval_ms, 0);
		EXPECT_EQ(stats1.pre_post_intervals[i].worst_update_interval_ms, 0);

		EXPECT_EQ(stats1.pre_inplay_intervals[i].num_updates, 0);
		EXPECT_DOUBLE_EQ(stats1.pre_inplay_intervals[i].mean_update_interval_ms, 0);
		EXPECT_EQ(stats1.pre_inplay_intervals[i].worst_update_interval_ms, 0);
	}
	EXPECT_EQ(stats1.inplay_intervals.num_updates, 0);
	EXPECT_DOUBLE_EQ(stats1.inplay_intervals.mean_update_interval_ms, 0);
	EXPECT_EQ(stats1.inplay_intervals.worst_update_interval_ms, 0);
}

// Test that we record that we saw at least 1 SP (we assume we would always get
// all not partial, which is generally how betfair send it).
TEST(stats_test, saw_sp)
{
	janus::dynamic_buffer dyn_buf(64);
	dyn_buf.add(janus::make_timestamp_update(1589178362508));
	dyn_buf.add(janus::make_timestamp_update(1589178462508));
	dyn_buf.add(janus::make_runner_id_update(123456));
	dyn_buf.add(janus::make_runner_sp_update(1.234));

	janus::stats stats1 = janus::generate_stats(nullptr, dyn_buf);
	EXPECT_EQ(dyn_buf.read_offset(), 64);
	EXPECT_EQ(stats1.flags, janus::stats_flags::SAW_SP);
	EXPECT_EQ(stats1.num_updates, 4);
	// Because we don't provide meta we work out how many runners.
	EXPECT_EQ(stats1.num_runners, 1);
	EXPECT_EQ(stats1.num_removals, 0);
	EXPECT_EQ(stats1.first_timestamp, 1589178362508);
	EXPECT_EQ(stats1.start_timestamp, 0);
	EXPECT_EQ(stats1.inplay_timestamp, 0);
	EXPECT_EQ(stats1.last_timestamp, 1589178462508);
	EXPECT_EQ(stats1.winner_runner_id, 0);
	for (uint64_t i = 0; i < janus::NUM_STATS_INTERVALS; i++) {
		EXPECT_EQ(stats1.pre_post_intervals[i].num_updates, 0);
		EXPECT_DOUBLE_EQ(stats1.pre_post_intervals[i].mean_update_interval_ms, 0);
		EXPECT_EQ(stats1.pre_post_intervals[i].worst_update_interval_ms, 0);

		EXPECT_EQ(stats1.pre_inplay_intervals[i].num_updates, 0);
		EXPECT_DOUBLE_EQ(stats1.pre_inplay_intervals[i].mean_update_interval_ms, 0);
		EXPECT_EQ(stats1.pre_inplay_intervals[i].worst_update_interval_ms, 0);
	}
	EXPECT_EQ(stats1.inplay_intervals.num_updates, 0);
	EXPECT_DOUBLE_EQ(stats1.inplay_intervals.mean_update_interval_ms, 0);
	EXPECT_EQ(stats1.inplay_intervals.worst_update_interval_ms, 0);
}

// Test that we record that we saw a winner and know its ID.
TEST(stats_test, saw_winner)
{
	janus::dynamic_buffer dyn_buf(80);
	dyn_buf.add(janus::make_timestamp_update(1589178362508));
	dyn_buf.add(janus::make_timestamp_update(1589178462508));
	// Make sure we are getting the right runner ID by providing 2.
	dyn_buf.add(janus::make_runner_id_update(999));
	dyn_buf.add(janus::make_runner_id_update(123456));
	dyn_buf.add(janus::make_runner_won_update());

	janus::stats stats1 = janus::generate_stats(nullptr, dyn_buf);
	EXPECT_EQ(dyn_buf.read_offset(), 80);
	EXPECT_EQ(stats1.flags, janus::stats_flags::SAW_WINNER);
	EXPECT_EQ(stats1.num_updates, 5);
	// Because we don't provide meta we work out how many runners.
	EXPECT_EQ(stats1.num_runners, 2);
	EXPECT_EQ(stats1.num_removals, 0);
	EXPECT_EQ(stats1.first_timestamp, 1589178362508);
	EXPECT_EQ(stats1.start_timestamp, 0);
	EXPECT_EQ(stats1.inplay_timestamp, 0);
	EXPECT_EQ(stats1.last_timestamp, 1589178462508);
	EXPECT_EQ(stats1.winner_runner_id, 123456);
	for (uint64_t i = 0; i < janus::NUM_STATS_INTERVALS; i++) {
		EXPECT_EQ(stats1.pre_post_intervals[i].num_updates, 0);
		EXPECT_DOUBLE_EQ(stats1.pre_post_intervals[i].mean_update_interval_ms, 0);
		EXPECT_EQ(stats1.pre_post_intervals[i].worst_update_interval_ms, 0);

		EXPECT_EQ(stats1.pre_inplay_intervals[i].num_updates, 0);
		EXPECT_DOUBLE_EQ(stats1.pre_inplay_intervals[i].mean_update_interval_ms, 0);
		EXPECT_EQ(stats1.pre_inplay_intervals[i].worst_update_interval_ms, 0);
	}
	EXPECT_EQ(stats1.inplay_intervals.num_updates, 0);
	EXPECT_DOUBLE_EQ(stats1.inplay_intervals.mean_update_interval_ms, 0);
	EXPECT_EQ(stats1.inplay_intervals.worst_update_interval_ms, 0);
}

// Test that the inplay timestamp gets picked up correctly.
TEST(stats_test, inplay_timestamp)
{
	janus::dynamic_buffer dyn_buf(48);
	dyn_buf.add(janus::make_timestamp_update(1589178362508));
	dyn_buf.add(janus::make_market_inplay_update());
	dyn_buf.add(janus::make_timestamp_update(1589178462508));

	janus::stats stats1 = janus::generate_stats(nullptr, dyn_buf);
	EXPECT_EQ(dyn_buf.read_offset(), 48);
	EXPECT_EQ(stats1.flags, janus::stats_flags::WENT_INPLAY);
	EXPECT_EQ(stats1.num_updates, 3);
	EXPECT_EQ(stats1.num_runners, 0);
	EXPECT_EQ(stats1.num_removals, 0);
	EXPECT_EQ(stats1.first_timestamp, 1589178362508);
	EXPECT_EQ(stats1.start_timestamp, 0);
	EXPECT_EQ(stats1.inplay_timestamp, 1589178362508);
	EXPECT_EQ(stats1.last_timestamp, 1589178462508);
	EXPECT_EQ(stats1.winner_runner_id, 0);
	for (uint64_t i = 0; i < janus::NUM_STATS_INTERVALS; i++) {
		EXPECT_EQ(stats1.pre_post_intervals[i].num_updates, 0);
		EXPECT_DOUBLE_EQ(stats1.pre_post_intervals[i].mean_update_interval_ms, 0);
		EXPECT_EQ(stats1.pre_post_intervals[i].worst_update_interval_ms, 0);

		EXPECT_EQ(stats1.pre_inplay_intervals[i].num_updates, 0);
		EXPECT_DOUBLE_EQ(stats1.pre_inplay_intervals[i].mean_update_interval_ms, 0);
		EXPECT_EQ(stats1.pre_inplay_intervals[i].worst_update_interval_ms, 0);
	}

	// Because we declared inplay immediately we get some inplay stats.
	EXPECT_EQ(stats1.inplay_intervals.num_updates, 2);
	EXPECT_DOUBLE_EQ(stats1.inplay_intervals.mean_update_interval_ms, 100000);
	EXPECT_EQ(stats1.inplay_intervals.worst_update_interval_ms, 100000);
}

// Test that inplay interval statistics are correct.
TEST(stats_test, inplay_interval)
{
	janus::dynamic_buffer dyn_buf(1024);
	dyn_buf.add(janus::make_timestamp_update(1589178362508));
	dyn_buf.add(janus::make_market_inplay_update());
	// + 100
	dyn_buf.add(janus::make_timestamp_update(1589178362608));
	// + 100
	dyn_buf.add(janus::make_timestamp_update(1589178362708));
	// + 200
	dyn_buf.add(janus::make_timestamp_update(1589178362908));
	// + 500
	dyn_buf.add(janus::make_timestamp_update(1589178363408));

	janus::stats stats1 = janus::generate_stats(nullptr, dyn_buf);
	EXPECT_EQ(stats1.flags, janus::stats_flags::WENT_INPLAY);
	EXPECT_EQ(stats1.num_updates, 6);
	EXPECT_EQ(stats1.num_runners, 0);
	EXPECT_EQ(stats1.num_removals, 0);
	EXPECT_EQ(stats1.first_timestamp, 1589178362508);
	EXPECT_EQ(stats1.start_timestamp, 0);
	EXPECT_EQ(stats1.inplay_timestamp, 1589178362508);
	EXPECT_EQ(stats1.last_timestamp, 1589178363408);
	EXPECT_EQ(stats1.winner_runner_id, 0);
	for (uint64_t i = 0; i < janus::NUM_STATS_INTERVALS; i++) {
		EXPECT_EQ(stats1.pre_post_intervals[i].num_updates, 0);
		EXPECT_DOUBLE_EQ(stats1.pre_post_intervals[i].mean_update_interval_ms, 0);
		EXPECT_EQ(stats1.pre_post_intervals[i].worst_update_interval_ms, 0);

		EXPECT_EQ(stats1.pre_inplay_intervals[i].num_updates, 0);
		EXPECT_DOUBLE_EQ(stats1.pre_inplay_intervals[i].mean_update_interval_ms, 0);
		EXPECT_EQ(stats1.pre_inplay_intervals[i].worst_update_interval_ms, 0);
	}

	// Because we declared inplay immediately we get some inplay stats.
	EXPECT_EQ(stats1.inplay_intervals.num_updates, 5);
	EXPECT_DOUBLE_EQ(stats1.inplay_intervals.mean_update_interval_ms, 225);
	EXPECT_EQ(stats1.inplay_intervals.worst_update_interval_ms, 500);
}

// Test that we can correctly calculate pre-post interval stats at the different time ranges.
TEST(stats_test, pre_post_intervals)
{
	// Calculated these values from spreadsheet of values :)

	janus::dynamic_buffer dyn_buf(1024);

	// Non-timestamp update should be ignored.
	dyn_buf.add(janus::make_runner_clear_unmatched());
	dyn_buf.add(janus::make_timestamp_update(1589178362508));
	dyn_buf.add(janus::make_timestamp_update(1589181962508));
	dyn_buf.add(janus::make_timestamp_update(1589185562508));
	// Arbitrary updates should just get counted.
	dyn_buf.add(janus::make_runner_clear_unmatched());
	dyn_buf.add(janus::make_timestamp_update(1589185563508));
	dyn_buf.add(janus::make_timestamp_update(1589185564508));
	dyn_buf.add(janus::make_timestamp_update(1589185566508));
	dyn_buf.add(janus::make_timestamp_update(1589185576508));
	dyn_buf.add(janus::make_timestamp_update(1589187362508));
	dyn_buf.add(janus::make_timestamp_update(1589187363508));
	// Add duplication, should be ignored.
	dyn_buf.add(janus::make_timestamp_update(1589187364508));
	dyn_buf.add(janus::make_timestamp_update(1589187364508));
	dyn_buf.add(janus::make_timestamp_update(1589188562508));
	dyn_buf.add(janus::make_timestamp_update(1589188562558));
	dyn_buf.add(janus::make_timestamp_update(1589188562608));
	dyn_buf.add(janus::make_timestamp_update(1589188862508));
	// Add duplication, should be ignored.
	dyn_buf.add(janus::make_timestamp_update(1589188862508));
	dyn_buf.add(janus::make_timestamp_update(1589188862608));
	dyn_buf.add(janus::make_timestamp_update(1589188862758));
	dyn_buf.add(janus::make_timestamp_update(1589188982508));
	dyn_buf.add(janus::make_timestamp_update(1589189102508));
	dyn_buf.add(janus::make_timestamp_update(1589189102558));
	dyn_buf.add(janus::make_timestamp_update(1589189102608));
	dyn_buf.add(janus::make_timestamp_update(1589189102808));
	dyn_buf.add(janus::make_timestamp_update(1589189162508));

	// We extract number of runners and market start time from the metadata.
	janus::meta_header header = {
		.market_start_timestamp = 1589189162508,
	};
	janus::meta_view meta(&header);
	meta.runners_mutable().push_back(janus::runner_view());
	meta.runners_mutable().push_back(janus::runner_view());

	janus::stats stats1 = janus::generate_stats(&meta, dyn_buf);
	EXPECT_EQ(stats1.flags, janus::stats_flags::HAVE_METADATA);
	EXPECT_EQ(stats1.num_updates, 26);
	EXPECT_EQ(stats1.num_runners, 2);
	EXPECT_EQ(stats1.num_removals, 0);
	EXPECT_EQ(stats1.first_timestamp, 1589178362508);
	EXPECT_EQ(stats1.start_timestamp, 1589189162508);
	EXPECT_EQ(stats1.inplay_timestamp, 0);
	EXPECT_EQ(stats1.last_timestamp, 1589189162508);
	EXPECT_EQ(stats1.winner_runner_id, 0);

	// Hour
	EXPECT_EQ(stats1.pre_post_intervals[0].num_updates, 20);
	EXPECT_DOUBLE_EQ(round_2dp(stats1.pre_post_intervals[0].mean_update_interval_ms),
			 189473.68);
	EXPECT_EQ(stats1.pre_post_intervals[0].worst_update_interval_ms, 1786000);

	// 30mins
	EXPECT_EQ(stats1.pre_post_intervals[1].num_updates, 14);
	EXPECT_DOUBLE_EQ(round_2dp(stats1.pre_post_intervals[1].mean_update_interval_ms),
			 128571.43);
	EXPECT_EQ(stats1.pre_post_intervals[1].worst_update_interval_ms, 1198000);

	// 10mins
	EXPECT_EQ(stats1.pre_post_intervals[2].num_updates, 11);
	EXPECT_DOUBLE_EQ(round_2dp(stats1.pre_post_intervals[2].mean_update_interval_ms), 54545.45);
	EXPECT_EQ(stats1.pre_post_intervals[2].worst_update_interval_ms, 299900);

	// 5mins
	EXPECT_EQ(stats1.pre_post_intervals[3].num_updates, 8);
	EXPECT_DOUBLE_EQ(round_2dp(stats1.pre_post_intervals[3].mean_update_interval_ms), 37500);
	EXPECT_EQ(stats1.pre_post_intervals[3].worst_update_interval_ms, 120000);

	// 3mins
	EXPECT_EQ(stats1.pre_post_intervals[4].num_updates, 5);
	EXPECT_DOUBLE_EQ(round_2dp(stats1.pre_post_intervals[4].mean_update_interval_ms), 36000);
	EXPECT_EQ(stats1.pre_post_intervals[4].worst_update_interval_ms, 120000);

	// 1min
	EXPECT_EQ(stats1.pre_post_intervals[5].num_updates, 4);
	EXPECT_DOUBLE_EQ(round_2dp(stats1.pre_post_intervals[5].mean_update_interval_ms), 15000);
	EXPECT_EQ(stats1.pre_post_intervals[5].worst_update_interval_ms, 59700);

	for (uint64_t i = 0; i < janus::NUM_STATS_INTERVALS; i++) {
		EXPECT_EQ(stats1.pre_inplay_intervals[i].num_updates, 0);
		EXPECT_DOUBLE_EQ(stats1.pre_inplay_intervals[i].mean_update_interval_ms, 0);
		EXPECT_EQ(stats1.pre_inplay_intervals[i].worst_update_interval_ms, 0);
	}
	EXPECT_EQ(stats1.inplay_intervals.num_updates, 0);
	EXPECT_DOUBLE_EQ(stats1.inplay_intervals.mean_update_interval_ms, 0);
	EXPECT_EQ(stats1.inplay_intervals.worst_update_interval_ms, 0);
}

// Test that we can correctly calculate pre-inplay interval stats at the different time ranges.
TEST(stats_test, pre_inplay_intervals)
{
	// Calculated these values from spreadsheet of values :)

	janus::dynamic_buffer dyn_buf(1024);

	// Non-timestamp update should be ignored.
	dyn_buf.add(janus::make_runner_clear_unmatched());
	dyn_buf.add(janus::make_timestamp_update(1589178362508));
	dyn_buf.add(janus::make_timestamp_update(1589181962508));
	dyn_buf.add(janus::make_timestamp_update(1589185562508));
	// Arbitrary updates should just get counted.
	dyn_buf.add(janus::make_runner_clear_unmatched());
	dyn_buf.add(janus::make_timestamp_update(1589185563508));
	dyn_buf.add(janus::make_timestamp_update(1589185564508));
	dyn_buf.add(janus::make_timestamp_update(1589185566508));
	dyn_buf.add(janus::make_timestamp_update(1589185576508));
	dyn_buf.add(janus::make_timestamp_update(1589187362508));
	dyn_buf.add(janus::make_timestamp_update(1589187363508));
	// Add duplication, should be ignored.
	dyn_buf.add(janus::make_timestamp_update(1589187364508));
	dyn_buf.add(janus::make_timestamp_update(1589187364508));
	dyn_buf.add(janus::make_timestamp_update(1589188562508));
	dyn_buf.add(janus::make_timestamp_update(1589188562558));
	dyn_buf.add(janus::make_timestamp_update(1589188562608));
	dyn_buf.add(janus::make_timestamp_update(1589188862508));
	// Add duplication, should be ignored.
	dyn_buf.add(janus::make_timestamp_update(1589188862508));
	dyn_buf.add(janus::make_timestamp_update(1589188862608));
	dyn_buf.add(janus::make_timestamp_update(1589188862758));
	dyn_buf.add(janus::make_timestamp_update(1589188982508));
	dyn_buf.add(janus::make_timestamp_update(1589189102508));
	dyn_buf.add(janus::make_timestamp_update(1589189102558));
	dyn_buf.add(janus::make_timestamp_update(1589189102608));
	dyn_buf.add(janus::make_timestamp_update(1589189102808));
	dyn_buf.add(janus::make_timestamp_update(1589189162508));
	// Now inplay :)
	dyn_buf.add(janus::make_market_inplay_update());

	janus::stats stats1 = janus::generate_stats(nullptr, dyn_buf);
	EXPECT_EQ(stats1.flags, janus::stats_flags::WENT_INPLAY);
	EXPECT_EQ(stats1.num_updates, 27);
	EXPECT_EQ(stats1.num_runners, 0);
	EXPECT_EQ(stats1.num_removals, 0);
	EXPECT_EQ(stats1.first_timestamp, 1589178362508);
	EXPECT_EQ(stats1.start_timestamp, 0);
	EXPECT_EQ(stats1.inplay_timestamp, 1589189162508);
	EXPECT_EQ(stats1.last_timestamp, 1589189162508);
	EXPECT_EQ(stats1.winner_runner_id, 0);

	// Hour
	EXPECT_EQ(stats1.pre_inplay_intervals[0].num_updates, 20);
	EXPECT_DOUBLE_EQ(round_2dp(stats1.pre_inplay_intervals[0].mean_update_interval_ms),
			 189473.68);
	EXPECT_EQ(stats1.pre_inplay_intervals[0].worst_update_interval_ms, 1786000);

	// 30mins
	EXPECT_EQ(stats1.pre_inplay_intervals[1].num_updates, 14);
	EXPECT_DOUBLE_EQ(round_2dp(stats1.pre_inplay_intervals[1].mean_update_interval_ms),
			 128571.43);
	EXPECT_EQ(stats1.pre_inplay_intervals[1].worst_update_interval_ms, 1198000);

	// 10mins
	EXPECT_EQ(stats1.pre_inplay_intervals[2].num_updates, 11);
	EXPECT_DOUBLE_EQ(round_2dp(stats1.pre_inplay_intervals[2].mean_update_interval_ms),
			 54545.45);
	EXPECT_EQ(stats1.pre_inplay_intervals[2].worst_update_interval_ms, 299900);

	// 5mins
	EXPECT_EQ(stats1.pre_inplay_intervals[3].num_updates, 8);
	EXPECT_DOUBLE_EQ(round_2dp(stats1.pre_inplay_intervals[3].mean_update_interval_ms), 37500);
	EXPECT_EQ(stats1.pre_inplay_intervals[3].worst_update_interval_ms, 120000);

	// 3mins
	EXPECT_EQ(stats1.pre_inplay_intervals[4].num_updates, 5);
	EXPECT_DOUBLE_EQ(round_2dp(stats1.pre_inplay_intervals[4].mean_update_interval_ms), 36000);
	EXPECT_EQ(stats1.pre_inplay_intervals[4].worst_update_interval_ms, 120000);

	// 1min
	EXPECT_EQ(stats1.pre_inplay_intervals[5].num_updates, 4);
	EXPECT_DOUBLE_EQ(round_2dp(stats1.pre_inplay_intervals[5].mean_update_interval_ms), 15000);
	EXPECT_EQ(stats1.pre_inplay_intervals[5].worst_update_interval_ms, 59700);

	for (uint64_t i = 0; i < janus::NUM_STATS_INTERVALS; i++) {
		EXPECT_EQ(stats1.pre_post_intervals[i].num_updates, 0);
		EXPECT_DOUBLE_EQ(stats1.pre_post_intervals[i].mean_update_interval_ms, 0);
		EXPECT_EQ(stats1.pre_post_intervals[i].worst_update_interval_ms, 0);
	}
	EXPECT_EQ(stats1.inplay_intervals.num_updates, 1);
	EXPECT_DOUBLE_EQ(stats1.inplay_intervals.mean_update_interval_ms, 0);
	EXPECT_EQ(stats1.inplay_intervals.worst_update_interval_ms, 0);
}
} // namespace
