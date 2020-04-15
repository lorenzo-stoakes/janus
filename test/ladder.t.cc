#include "janus.hh"

#include <algorithm>
#include <array>
#include <cmath>
#include <gtest/gtest.h>
#include <utility>

namespace
{
// Test that we can add and read unmatched back and lay volume.
TEST(ladder_test, unmatched)
{
	janus::betfair::ladder ladder;

	// Fill half of ladder with ATB volume.
	for (uint64_t price_index = 0; price_index < janus::betfair::NUM_PRICES / 2;
	     price_index++) {
		double vol = price_index;
		vol *= -100; // Negative for ATB.

		ladder.set_unmatched_at(price_index, vol);
		EXPECT_DOUBLE_EQ(ladder.unmatched(price_index), vol);
		EXPECT_DOUBLE_EQ(ladder[price_index], vol);
	}

	// Fill the other half with ATL volume.
	for (uint64_t price_index = janus::betfair::NUM_PRICES / 2;
	     price_index < janus::betfair::NUM_PRICES; price_index++) {
		double vol = price_index;
		vol *= 100;

		ladder.set_unmatched_at(price_index, vol);
		EXPECT_DOUBLE_EQ(ladder.unmatched(price_index), vol);
		EXPECT_DOUBLE_EQ(ladder[price_index], vol);
	}

	// Now make sure entire ladder is set as expected.
	for (uint64_t price_index = 0; price_index < janus::betfair::NUM_PRICES; price_index++) {
		EXPECT_DOUBLE_EQ(::fabs(ladder[price_index]), price_index * 100);
		EXPECT_DOUBLE_EQ(::fabs(ladder.unmatched(price_index)), price_index * 100);
	}

	// We shouldn't be able to set discontinuities in the ladder.

	for (uint64_t price_index = 0; price_index < janus::betfair::NUM_PRICES / 2 - 1;
	     price_index++) {
		ASSERT_THROW(ladder.set_unmatched_at(price_index, 100),
			     janus::betfair::invalid_unmatched_update);
	}

	for (uint64_t price_index = janus::betfair::NUM_PRICES / 2 + 1;
	     price_index < janus::betfair::NUM_PRICES; price_index++) {
		ASSERT_THROW(ladder.set_unmatched_at(price_index, -100),
			     janus::betfair::invalid_unmatched_update);
	}

	// But we should be able to incrementally increase and decrease the
	// middle of the ladder by replacing the max ATB/min ATL.
	for (uint64_t price_index = janus::betfair::NUM_PRICES / 2 - 1; price_index > 0;
	     price_index--) {
		ASSERT_NO_THROW(ladder.set_unmatched_at(price_index, 100));
	}
	for (uint64_t price_index = 1; price_index < janus::betfair::NUM_PRICES - 1;
	     price_index++) {
		ASSERT_NO_THROW(ladder.set_unmatched_at(price_index, -100));
	}
}

// Test that .min_atl_index() and .max_atb_index() correctly return minimum ATL
// index and maximum ATB index respectively.
TEST(ladder_test, limit_indexes)
{
	janus::betfair::ladder ladder;

	// Fill half of ladder with ATB volume.
	for (uint64_t price_index = 0; price_index < janus::betfair::NUM_PRICES / 2;
	     price_index++) {
		double vol = price_index;
		vol *= -100; // Negative for ATB.

		ladder.set_unmatched_at(price_index, vol);
	}

	// Fill the other half with ATL volume.
	for (uint64_t price_index = janus::betfair::NUM_PRICES / 2;
	     price_index < janus::betfair::NUM_PRICES; price_index++) {
		double vol = price_index;
		vol *= 100;

		ladder.set_unmatched_at(price_index, vol);
	}

	uint64_t expected_max_atb_index = janus::betfair::NUM_PRICES / 2 - 1;
	uint64_t expected_min_atl_index = janus::betfair::NUM_PRICES / 2;

	// Limit indexes should be right in the middle.
	EXPECT_EQ(ladder.max_atb_index(), expected_max_atb_index);
	EXPECT_EQ(ladder.min_atl_index(), expected_min_atl_index);

	// Incremently replacing lays with backs should drive both the max ATB
	// and min ATL down.
	for (uint64_t price_index = janus::betfair::NUM_PRICES / 2 - 1; price_index > 0;
	     price_index--) {
		ladder.set_unmatched_at(price_index, 100);
		EXPECT_EQ(ladder.max_atb_index(), --expected_max_atb_index);
		EXPECT_EQ(ladder.min_atl_index(), --expected_min_atl_index);
	}

	// Incrementing replaying backs with lays should drive both the max ATB
	// and min ATL up.
	for (uint64_t price_index = 1; price_index < janus::betfair::NUM_PRICES - 1;
	     price_index++) {
		ladder.set_unmatched_at(price_index, -100);
		EXPECT_EQ(ladder.max_atb_index(), ++expected_max_atb_index);
		EXPECT_EQ(ladder.min_atl_index(), ++expected_min_atl_index);
	}

	// Clearing lays should drive the max ATB down but leave the min ATL
	// where it is.
	for (uint64_t price_index = janus::betfair::NUM_PRICES - 2; price_index > 0;
	     price_index--) {
		ladder.clear_unmatched_at(price_index);
		EXPECT_EQ(ladder.max_atb_index(), --expected_max_atb_index);
		EXPECT_EQ(ladder.min_atl_index(), expected_min_atl_index);
	}

	// We should be able to arbitrarily set backs and have the min ATL move
	// to the minimum of these.

	ladder.set_unmatched_at(20, 100);
	EXPECT_EQ(ladder.min_atl_index(), 20);

	ladder.set_unmatched_at(40, 100);
	EXPECT_EQ(ladder.min_atl_index(), 20);

	ladder.set_unmatched_at(100, 100);
	EXPECT_EQ(ladder.min_atl_index(), 20);

	ladder.set_unmatched_at(200, 100);
	EXPECT_EQ(ladder.min_atl_index(), 20);

	ladder.clear_unmatched_at(20);
	EXPECT_EQ(ladder.min_atl_index(), 40);

	ladder.clear_unmatched_at(40);
	EXPECT_EQ(ladder.min_atl_index(), 100);

	ladder.clear_unmatched_at(100);
	EXPECT_EQ(ladder.min_atl_index(), 200);

	ladder.set_unmatched_at(300, 100);
	EXPECT_EQ(ladder.min_atl_index(), 200);

	ladder.set_unmatched_at(320, 100);
	EXPECT_EQ(ladder.min_atl_index(), 200);

	ladder.clear_unmatched_at(320);
	EXPECT_EQ(ladder.min_atl_index(), 200);

	ladder.clear_unmatched_at(200);
	EXPECT_EQ(ladder.min_atl_index(), 300);

	ladder.clear_unmatched_at(300);
	EXPECT_EQ(ladder.min_atl_index(), janus::betfair::NUM_PRICES - 1);

	// Equally, for lays:

	ladder.set_unmatched_at(100, -100);
	EXPECT_EQ(ladder.max_atb_index(), 100);

	ladder.set_unmatched_at(40, -100);
	EXPECT_EQ(ladder.max_atb_index(), 100);

	ladder.set_unmatched_at(20, -100);
	EXPECT_EQ(ladder.max_atb_index(), 100);

	ladder.set_unmatched_at(10, -100);
	EXPECT_EQ(ladder.max_atb_index(), 100);

	ladder.clear_unmatched_at(100);
	EXPECT_EQ(ladder.max_atb_index(), 40);

	ladder.clear_unmatched_at(40);
	EXPECT_EQ(ladder.max_atb_index(), 20);

	ladder.clear_unmatched_at(20);
	EXPECT_EQ(ladder.max_atb_index(), 10);

	ladder.set_unmatched_at(5, -100);
	EXPECT_EQ(ladder.max_atb_index(), 10);

	ladder.set_unmatched_at(4, -100);
	EXPECT_EQ(ladder.max_atb_index(), 10);

	ladder.clear_unmatched_at(4);
	EXPECT_EQ(ladder.max_atb_index(), 10);

	ladder.clear_unmatched_at(10);
	EXPECT_EQ(ladder.max_atb_index(), 5);

	ladder.clear_unmatched_at(5);
	EXPECT_EQ(ladder.max_atb_index(), 0);

	// If we set a a couple max ATBs and min ATLs then replace one with the
	// other, we should correctly update both.

	ladder.set_unmatched_at(50, -100);
	EXPECT_EQ(ladder.max_atb_index(), 50);

	ladder.set_unmatched_at(100, 100);
	EXPECT_EQ(ladder.min_atl_index(), 100);

	ladder.set_unmatched_at(100, -100);
	EXPECT_EQ(ladder.max_atb_index(), 100);
	EXPECT_EQ(ladder.min_atl_index(), janus::betfair::NUM_PRICES - 1);

	ladder.set_unmatched_at(100, 100);
	EXPECT_EQ(ladder.max_atb_index(), 50);
	EXPECT_EQ(ladder.min_atl_index(), 100);

	ladder.set_unmatched_at(50, 100);
	EXPECT_EQ(ladder.min_atl_index(), 50);
	EXPECT_EQ(ladder.max_atb_index(), 0);
}

// Test that .total_unmatched_atb() and .total_unmatched_atl() correctly track
// total unmatched volumes.
TEST(ladder_test, total_unmatched)
{
	janus::betfair::ladder ladder;

	double expected_total_atb = 0;
	double expected_total_atl = 0;

	// Fill half of ladder with ATB volume.
	for (uint64_t price_index = 0; price_index < janus::betfair::NUM_PRICES / 2;
	     price_index++) {
		double vol = price_index;
		vol *= -100; // Negative for ATB.
		expected_total_atb -= vol;

		ladder.set_unmatched_at(price_index, vol);
	}

	// Fill the other half with ATL volume.
	for (uint64_t price_index = janus::betfair::NUM_PRICES / 2;
	     price_index < janus::betfair::NUM_PRICES; price_index++) {
		double vol = price_index;
		vol *= 100;
		expected_total_atl += vol;

		ladder.set_unmatched_at(price_index, vol);
	}

	EXPECT_EQ(ladder.total_unmatched_atb(), expected_total_atb);
	EXPECT_EQ(ladder.total_unmatched_atl(), expected_total_atl);

	// Now replace all values with 100 at each price level and expect total
	// ATB/ATL figures to be updated accordingly.

	expected_total_atb = 0;
	for (uint64_t price_index = 0; price_index < janus::betfair::NUM_PRICES / 2;
	     price_index++) {
		ladder.set_unmatched_at(price_index, -100);
		expected_total_atb += 100;
	}

	expected_total_atl = 0;
	for (uint64_t price_index = janus::betfair::NUM_PRICES / 2;
	     price_index < janus::betfair::NUM_PRICES; price_index++) {
		ladder.set_unmatched_at(price_index, 100);
		expected_total_atl += 100;
	}

	EXPECT_EQ(ladder.total_unmatched_atb(), expected_total_atb);
	EXPECT_EQ(ladder.total_unmatched_atl(), expected_total_atl);

	ladder.set_unmatched_at(340, 50);
	expected_total_atl -= 50;
	EXPECT_EQ(ladder.total_unmatched_atl(), expected_total_atl);

	ladder.clear_unmatched_at(20);
	expected_total_atb -= 100;
	EXPECT_EQ(ladder.total_unmatched_atb(), expected_total_atb);

	ladder.set_unmatched_at(340, 250);
	expected_total_atl += 200;
	EXPECT_EQ(ladder.total_unmatched_atl(), expected_total_atl);
}

// Test that .clear() correctly clears down the ladder.
TEST(ladder_test, clear)
{
	janus::betfair::ladder ladder;

	double expected_total_atb = 0;
	double expected_total_atl = 0;

	// Fill half of ladder with ATB volume.
	for (uint64_t price_index = 0; price_index < janus::betfair::NUM_PRICES / 2;
	     price_index++) {
		double vol = price_index;
		vol *= -100; // Negative for ATB.
		expected_total_atb -= vol;

		ladder.set_unmatched_at(price_index, vol);
	}

	// Fill the other half with ATL volume.
	for (uint64_t price_index = janus::betfair::NUM_PRICES / 2;
	     price_index < janus::betfair::NUM_PRICES; price_index++) {
		double vol = price_index;
		vol *= 100;
		expected_total_atl += vol;

		ladder.set_unmatched_at(price_index, vol);
	}

	double expected_total_matched = 0;
	for (uint64_t price_index = 0; price_index < janus::betfair::NUM_PRICES; price_index++) {
		double vol = price_index;
		vol *= 100;
		expected_total_matched += vol;

		ladder.set_matched_at(price_index, vol);
	}

	EXPECT_EQ(ladder.total_unmatched_atb(), expected_total_atb);
	EXPECT_EQ(ladder.total_unmatched_atl(), expected_total_atl);
	EXPECT_EQ(ladder.total_matched(), expected_total_matched);
	EXPECT_EQ(ladder.max_atb_index(), janus::betfair::NUM_PRICES / 2 - 1);
	EXPECT_EQ(ladder.min_atl_index(), janus::betfair::NUM_PRICES / 2);

	ladder.clear();

	EXPECT_EQ(ladder.total_unmatched_atb(), 0);
	EXPECT_EQ(ladder.total_unmatched_atl(), 0);
	EXPECT_EQ(ladder.total_matched(), 0);
	EXPECT_EQ(ladder.max_atb_index(), 0);
	EXPECT_EQ(ladder.min_atl_index(), janus::betfair::NUM_PRICES - 1);

	for (uint64_t price_index = 0; price_index < janus::betfair::NUM_PRICES; price_index++) {
		ASSERT_EQ(ladder[price_index], 0);
		ASSERT_EQ(ladder.matched(price_index), 0);
	}
}

// Test that .best_atl() and .best_atb() correctly retrieves the top N ATL and
// ATB unmatched volume available.
TEST(ladder_test, best_atl_atb)
{
	janus::betfair::ladder ladder;

	std::array<uint64_t, 10> prices;
	std::array<double, 10> vols;

	// Should start empty.
	EXPECT_EQ(ladder.best_atb(10, prices.data(), vols.data()), 0);
	EXPECT_EQ(ladder.best_atl(10, prices.data(), vols.data()), 0);

	auto zero_pair = std::make_pair<uint64_t, double>(0, 0);
	EXPECT_EQ(ladder.best_atb(), zero_pair);
	EXPECT_EQ(ladder.best_atl(), zero_pair);

	// Fill half of ladder with ATB volume.
	for (uint64_t price_index = 0; price_index < janus::betfair::NUM_PRICES / 2;
	     price_index++) {
		double vol = price_index == 0 ? 1 : price_index;
		vol *= -100; // Negative for ATB.

		ladder.set_unmatched_at(price_index, vol);

		// Test as we go.
		uint64_t count = ladder.best_atb(10, prices.data(), vols.data());
		ASSERT_EQ(count, std::min<uint64_t>(price_index + 1, 10));
		for (uint64_t i = 0; i < count; i++) {
			uint64_t expected_price_index = price_index - i;
			double expected_vol = expected_price_index == 0 ? 1 : expected_price_index;
			expected_vol *= 100;

			ASSERT_EQ(prices[i], expected_price_index);
			ASSERT_DOUBLE_EQ(vols[i], expected_vol);
		}
	}

	// Fill the other half with ATL volume.
	for (uint64_t price_index = janus::betfair::NUM_PRICES / 2;
	     price_index < janus::betfair::NUM_PRICES; price_index++) {
		double vol = price_index;
		vol *= 100;

		ladder.set_unmatched_at(price_index, vol);

		// Test as we go.
		uint64_t count = ladder.best_atl(10, prices.data(), vols.data());
		ASSERT_EQ(count,
			  std::min<uint64_t>(price_index - janus::betfair::NUM_PRICES / 2 + 1, 10));
		for (uint64_t i = 0; i < count; i++) {
			uint64_t expected_price_index = janus::betfair::NUM_PRICES / 2 + i;
			double expected_vol = expected_price_index;
			expected_vol *= 100;

			ASSERT_EQ(prices[i], expected_price_index);
			ASSERT_DOUBLE_EQ(vols[i], expected_vol);
		}
	}

	// Now test after we have added values.
	for (uint64_t i = 1; i <= 10; i++) {
		uint64_t count = ladder.best_atl(i, prices.data(), vols.data());
		EXPECT_EQ(count, i);

		for (uint64_t j = 0; j < i; j++) {
			uint64_t price_index = janus::betfair::NUM_PRICES / 2 + j;
			double vol = price_index * 100;

			ASSERT_EQ(prices[j], price_index);
			ASSERT_DOUBLE_EQ(vols[j], vol);

			if (j == 0) {
				ASSERT_EQ(ladder.best_atl(), std::make_pair(price_index, vol));
			}
		}

		count = ladder.best_atb(i, prices.data(), vols.data());
		EXPECT_EQ(count, i);

		for (uint64_t j = 0; j < i; j++) {
			uint64_t price_index = janus::betfair::NUM_PRICES / 2 - 1 - j;
			double vol = price_index;
			vol *= 100;

			ASSERT_EQ(prices[j], price_index);
			ASSERT_DOUBLE_EQ(vols[j], vol);

			if (j == 0) {
				ASSERT_EQ(ladder.best_atb(), std::make_pair(price_index, vol));
			}
		}
	}

	// Now test a gappy ladder.

	janus::betfair::ladder ladder2;

	ladder2.set_unmatched_at(10, -123);
	ladder2.set_unmatched_at(15, 456);
	ladder2.set_unmatched_at(3, -12);
	ladder2.set_unmatched_at(100, 789);
	ladder2.set_unmatched_at(349, 1024);

	EXPECT_EQ(ladder2.best_atb(10, prices.data(), vols.data()), 2);

	EXPECT_EQ(prices[0], 10);
	EXPECT_EQ(vols[0], 123);

	EXPECT_EQ(prices[1], 3);
	EXPECT_EQ(vols[1], 12);

	EXPECT_EQ(ladder2.best_atl(10, prices.data(), vols.data()), 3);

	EXPECT_EQ(prices[0], 15);
	EXPECT_EQ(vols[0], 456);

	EXPECT_EQ(prices[1], 100);
	EXPECT_EQ(vols[1], 789);

	EXPECT_EQ(prices[2], 349);
	EXPECT_EQ(vols[2], 1024);

	// Make sure it updates correctly after a clear.

	ladder2.clear_unmatched_at(10);
	EXPECT_EQ(ladder2.best_atb(10, prices.data(), vols.data()), 1);

	EXPECT_EQ(prices[0], 3);
	EXPECT_EQ(vols[0], 12);

	ladder2.clear_unmatched_at(15);
	EXPECT_EQ(ladder2.best_atl(10, prices.data(), vols.data()), 2);

	EXPECT_EQ(prices[0], 100);
	EXPECT_EQ(vols[0], 789);

	// Make sure updates work.

	ladder2.set_unmatched_at(100, 5000);
	EXPECT_EQ(ladder2.best_atl(10, prices.data(), vols.data()), 2);

	EXPECT_EQ(prices[0], 100);
	EXPECT_EQ(vols[0], 5000);

	ladder2.set_unmatched_at(10, -761);
	EXPECT_EQ(ladder2.best_atb(10, prices.data(), vols.data()), 2);

	EXPECT_EQ(prices[0], 10);
	EXPECT_EQ(vols[0], 761);
}

// Test that list initialiser constructor works correctly assigning price index,
// volume pairs.
TEST(ladder_test, list_init_ctor)
{
	janus::betfair::ladder ladder = {
		{123, -456.12}, {210, -99.34}, {620, 597.68}, {640, 999.78}};

	janus::betfair::price_range range;

	EXPECT_DOUBLE_EQ(ladder[range.pricex100_to_index(123)], -456.12);
	EXPECT_DOUBLE_EQ(ladder[range.pricex100_to_index(210)], -99.34);

	EXPECT_DOUBLE_EQ(ladder[range.pricex100_to_index(620)], 597.68);
	EXPECT_DOUBLE_EQ(ladder[range.pricex100_to_index(640)], 999.78);
}

// Test that we can add, read and clear matched volume.
TEST(ladder_test, matched)
{
	janus::betfair::ladder ladder;

	for (uint64_t price_index = 0; price_index < janus::betfair::NUM_PRICES; price_index++) {
		double vol = price_index * 100;

		ladder.set_matched_at(price_index, vol);
	}

	for (uint64_t price_index = 0; price_index < janus::betfair::NUM_PRICES; price_index++) {
		double vol = price_index * 100;

		ASSERT_EQ(ladder.matched(price_index), vol);
	}

	for (uint64_t price_index = 0; price_index < janus::betfair::NUM_PRICES; price_index++) {
		ladder.clear_matched_at(price_index);
		ASSERT_EQ(ladder.matched(price_index), 0);
	}
}

// Test that .total_matched() correctly tracks totalnmatched volumes.
TEST(ladder_test, total_matched)
{
	janus::betfair::ladder ladder;

	double expected_total_matched = 0;
	for (uint64_t price_index = 0; price_index < janus::betfair::NUM_PRICES; price_index++) {
		double vol = price_index * 100;
		;
		expected_total_matched += vol;

		ladder.set_matched_at(price_index, vol);
	}

	EXPECT_EQ(ladder.total_matched(), expected_total_matched);

	// Test that clearing at a price level correctly updates total matched volume.

	for (uint64_t price_index = 0; price_index < janus::betfair::NUM_PRICES; price_index++) {
		double vol = price_index * 100;
		expected_total_matched -= vol;

		ladder.clear_matched_at(price_index);
		ASSERT_EQ(ladder.total_matched(), expected_total_matched);
	}
	ASSERT_EQ(ladder.total_matched(), 0);
}
} // namespace
