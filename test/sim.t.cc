#include "janus.hh"

#include <cmath>
#include <gtest/gtest.h>

namespace
{
// Quick and dirty function to round a number to 2 decimal places.
// TODO: Duplicated!
static double round_2dp(double n)
{
	uint64_t nx100 = ::llround(n * 100.);
	return static_cast<double>(nx100) / 100;
}

// Test that basic sim functionality works correctly.
TEST(sim_test, basic)
{
	janus::betfair::price_range range;

	janus::betfair::market market1(123456);
	janus::betfair::runner& runner1 = market1.add_runner(123);
	janus::betfair::ladder& ladder1 = runner1.ladder();
	ladder1.set_unmatched_at(range.price_to_nearest_index(6.4), 123.45);
	ladder1.set_unmatched_at(range.price_to_nearest_index(6.2), -100.5);

	auto sim = std::make_unique<janus::sim>(range, market1);

	// Back at 1.01 should get matched right away.
	janus::bet* bet1 = sim->add_bet(123, 1.01, 1000, true);
	ASSERT_NE(bet1, nullptr);
	EXPECT_EQ(&sim->bets()[0], bet1);
	EXPECT_EQ(bet1->flags(), janus::bet_flags::SIM | janus::bet_flags::ACKED);
	EXPECT_EQ(bet1->bet_id(), 0);
	EXPECT_EQ(bet1->runner_id(), 123);
	// We should get best price available.
	EXPECT_DOUBLE_EQ(bet1->price(), 6.2);
	EXPECT_DOUBLE_EQ(bet1->stake(), 1000);
	EXPECT_TRUE(bet1->is_back());
	// Since we're backing at lay level we should be insta-matched.
	EXPECT_DOUBLE_EQ(bet1->matched(), 1000);
	EXPECT_DOUBLE_EQ(bet1->unmatched(), 0);
	EXPECT_TRUE(bet1->is_complete());

	// Add some matched volume at 6.4.
	ladder1.set_matched_at(range.price_to_nearest_index(6.4), 53.2);

	// Now back at 6.4, this shouldn't match.
	janus::bet* bet2 = sim->add_bet(123, 6.4, 1000, true);
	ASSERT_NE(bet2, nullptr);
	EXPECT_EQ(&sim->bets()[1], bet2);
	EXPECT_EQ(bet2->bet_id(), 1);
	EXPECT_DOUBLE_EQ(bet2->price(), 6.4);
	EXPECT_DOUBLE_EQ(bet2->unmatched(), 1000);
	EXPECT_DOUBLE_EQ(bet2->matched(), 0);
	EXPECT_FALSE(bet2->is_complete());
	// Target should be twice queue plus matched.
	EXPECT_DOUBLE_EQ(bet2->target_matched(), 2 * 123.45 + 53.2);

	// Set matched to target + 10.3.
	ladder1.set_matched_at(range.price_to_nearest_index(6.4), 2 * 123.45 + 53.2 + 10.3);

	// Update sim to pick it up.
	sim->update();

	// Should have matched 5.15 (half of 10.3).
	EXPECT_DOUBLE_EQ(round_2dp(bet2->matched()), 5.15);
	EXPECT_DOUBLE_EQ(round_2dp(bet2->unmatched()), 994.85);

	// Our first bet should be unaffected.
	EXPECT_DOUBLE_EQ(bet1->matched(), 1000);
	EXPECT_DOUBLE_EQ(bet1->unmatched(), 0);

	// Update again - shouldn't have any bearing.
	sim->update();
	EXPECT_DOUBLE_EQ(round_2dp(bet2->matched()), 5.15);

	janus::betfair::runner& runner2 = market1.add_runner(456);
	janus::betfair::ladder& ladder2 = runner2.ladder();
	ladder2.set_unmatched_at(range.price_to_nearest_index(6.4), 123.45);

	janus::bet* bet3 = sim->add_bet(456, 1000, 500, false);
	ASSERT_NE(bet3, nullptr);
	EXPECT_EQ(&sim->bets()[2], bet3);
	EXPECT_EQ(bet1->flags(), janus::bet_flags::SIM | janus::bet_flags::ACKED);
	// Lay at 1000, should get matched right away at best price.
	EXPECT_DOUBLE_EQ(bet3->price(), 6.4);
	EXPECT_TRUE(bet3->is_complete());
	EXPECT_DOUBLE_EQ(bet3->stake(), 500);
	EXPECT_DOUBLE_EQ(bet3->matched(), 500);

	// Now remove runner 2 with a 7.9% reduction factor.
	runner2.set_removed(7.9);

	// Update to pick up removal.
	sim->update();

	// Our bet on runner 2 should be voided.
	EXPECT_EQ(bet3->flags(),
		  janus::bet_flags::SIM | janus::bet_flags::ACKED | janus::bet_flags::VOIDED);
	EXPECT_DOUBLE_EQ(bet3->stake(), 0);
	EXPECT_DOUBLE_EQ(bet3->matched(), 0);
	EXPECT_TRUE(bet3->is_complete());

	// Other bets prices should be scaled.
	EXPECT_DOUBLE_EQ(bet1->price(), 6.2 * (1. - 0.079));
	EXPECT_DOUBLE_EQ(bet2->price(), 6.4 * (1. - 0.079));

	// Trying to add to a removed runner shouldn't work.
	EXPECT_EQ(sim->add_bet(456, 6.4, 1000, false), nullptr);
	ASSERT_EQ(sim->bets().size(), 4);

	// We shouldn't be able to add a bet to an inplay market either.
	market1.set_inplay(true);
	EXPECT_EQ(sim->add_bet(123, 6.4, 1000, false), nullptr);
	ASSERT_EQ(sim->bets().size(), 4);

	// bet 2 should have had its unmatched portion split into a new bet.
	EXPECT_DOUBLE_EQ(bet2->unmatched(), 0);
	ASSERT_EQ(sim->bets().size(), 4);
	janus::bet* bet4 = &sim->bets()[3];
	EXPECT_EQ(bet4->runner_id(), 123);
	EXPECT_DOUBLE_EQ(bet4->price(), 6.4);
	EXPECT_TRUE(bet4->is_back());
	EXPECT_DOUBLE_EQ(round_2dp(bet4->stake()), 994.85);
	EXPECT_DOUBLE_EQ(bet4->matched(), 0);
	EXPECT_DOUBLE_EQ(round_2dp(bet4->unmatched()), 994.85);

	// Now calculate P/L if runner 1 wins.

	// Before we set the winner the method should return 0.
	EXPECT_DOUBLE_EQ(sim->pl(), 0);
	runner1.set_won();
	// Otherwise expect correct scaled value.
	EXPECT_DOUBLE_EQ(round_2dp(sim->pl()), round_2dp(1000 * (6.2 * (1. - 0.079) - 1) +
							 5.15 * (6.4 * (1 - 0.079) - 1)));
}

// Test that the hedge() function correctly hedges bets.
TEST(sim_test, hedge)
{
	janus::betfair::price_range range;

	janus::betfair::market market1(123456);
	janus::betfair::runner& runner1 = market1.add_runner(123);
	janus::betfair::ladder& ladder1 = runner1.ladder();

	auto sim = std::make_unique<janus::sim>(range, market1);

	// Backs: 90 @ 7.2, 30 @ 6.4.
	ladder1.set_unmatched_at(range.price_to_nearest_index(7.2), -90);
	sim->add_bet(123, 7.2, 90, true);
	ladder1.clear_unmatched_at(range.price_to_nearest_index(7.2));
	ladder1.set_unmatched_at(range.price_to_nearest_index(6.4), -30);
	sim->add_bet(123, 6.4, 30, true);
	ladder1.clear_unmatched_at(range.price_to_nearest_index(6.4));

	// Lays: 50 @ 6.2, 30 @ 6.
	ladder1.set_unmatched_at(range.price_to_nearest_index(6.2), 50);
	sim->add_bet(123, 6.2, 50, false);
	ladder1.clear_unmatched_at(range.price_to_nearest_index(6.2));
	ladder1.set_unmatched_at(range.price_to_nearest_index(6), 30);
	sim->add_bet(123, 6, 30, false);
	ladder1.clear_unmatched_at(range.price_to_nearest_index(6));

	EXPECT_EQ(sim->bets().size(), 4);

	// Now hedge @ 5.8.
	ladder1.set_unmatched_at(range.price_to_nearest_index(5.8), 61);
	// We set price 980 but we should get scaled to 5.8 correctly.
	EXPECT_TRUE(sim->hedge(123, 980));
	ASSERT_EQ(sim->bets().size(), 5);
	ladder1.clear_unmatched_at(range.price_to_nearest_index(5.8));
	janus::bet& bet = sim->bets()[4];
	EXPECT_EQ(bet.runner_id(), 123);
	EXPECT_FALSE(bet.is_back());
	EXPECT_DOUBLE_EQ(bet.price(), 5.8);
	EXPECT_DOUBLE_EQ(round_2dp(bet.stake()), 60.34);

	// A further hedge should fail.
	ladder1.set_unmatched_at(range.price_to_nearest_index(5.8), 61);
	EXPECT_FALSE(sim->hedge(123, 5.8));
	EXPECT_EQ(sim->bets().size(), 5);
}
} // namespace
