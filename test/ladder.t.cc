#include "ladder.hh"

#include <cmath>
#include <gtest/gtest.h>

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

	EXPECT_EQ(ladder.total_unmatched_atb(), expected_total_atb);
	EXPECT_EQ(ladder.total_unmatched_atl(), expected_total_atl);
	EXPECT_EQ(ladder.max_atb_index(), janus::betfair::NUM_PRICES / 2 - 1);
	EXPECT_EQ(ladder.min_atl_index(), janus::betfair::NUM_PRICES / 2);

	ladder.clear();

	EXPECT_EQ(ladder.total_unmatched_atb(), 0);
	EXPECT_EQ(ladder.total_unmatched_atl(), 0);
	EXPECT_EQ(ladder.max_atb_index(), 0);
	EXPECT_EQ(ladder.min_atl_index(), janus::betfair::NUM_PRICES - 1);
}
} // namespace
