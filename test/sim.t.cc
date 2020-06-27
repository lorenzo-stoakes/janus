#include "janus.hh"
#include "test_util.hh"

#include <gtest/gtest.h>

namespace
{
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

// Test that the hedge() function works correctly with unspecified price (use
// best available).
TEST(sim_test, hedge_unspecified_price)
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
	// We set -1 to get best available price (5.8).
	EXPECT_TRUE(sim->hedge(123, -1));
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

// Test that the hedge() function works correctly with unspecified price (use
// best available) with back hedge bet.
TEST(sim_test, hedge_unspecified_price_back)
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

	// Lays: 50 @ 6.2, 100 @ 6.
	ladder1.set_unmatched_at(range.price_to_nearest_index(6.2), 50);
	sim->add_bet(123, 6.2, 50, false);
	ladder1.clear_unmatched_at(range.price_to_nearest_index(6.2));
	ladder1.set_unmatched_at(range.price_to_nearest_index(6), 100);
	sim->add_bet(123, 6, 100, false);
	ladder1.clear_unmatched_at(range.price_to_nearest_index(6));

	EXPECT_EQ(sim->bets().size(), 4);

	// Now hedge @ unspecified price (should resolve to 5.8).
	ladder1.set_unmatched_at(range.price_to_nearest_index(5.8), -1);
	EXPECT_TRUE(sim->hedge(123));
	ASSERT_EQ(sim->bets().size(), 5);
	ladder1.clear_unmatched_at(range.price_to_nearest_index(5.8));
	janus::bet& bet = sim->bets()[4];
	EXPECT_EQ(bet.runner_id(), 123);
	EXPECT_TRUE(bet.is_back());
	EXPECT_DOUBLE_EQ(bet.price(), 5.8);
	EXPECT_DOUBLE_EQ(round_2dp(bet.stake()), 12.07);

	// A further hedge should fail.
	ladder1.set_unmatched_at(range.price_to_nearest_index(5.8), 61);
	EXPECT_FALSE(sim->hedge(123, 5.8));
	EXPECT_EQ(sim->bets().size(), 5);
}

// Test that the hedge() function works correctly with unspecified price (use
// best available) with back hedge bet.
TEST(sim_test, hedge_unspecified_price_lay)
{
	janus::betfair::price_range range;

	janus::betfair::market market1(123456);
	janus::betfair::runner& runner1 = market1.add_runner(123);
	janus::betfair::ladder& ladder1 = runner1.ladder();

	auto sim = std::make_unique<janus::sim>(range, market1);

	// Backs: 190 @ 7.2, 30 @ 6.4.
	ladder1.set_unmatched_at(range.price_to_nearest_index(7.2), -190);
	sim->add_bet(123, 7.2, 190, true);
	ladder1.clear_unmatched_at(range.price_to_nearest_index(7.2));
	ladder1.set_unmatched_at(range.price_to_nearest_index(6.4), -30);
	sim->add_bet(123, 6.4, 30, true);
	ladder1.clear_unmatched_at(range.price_to_nearest_index(6.4));

	// Lays: 50 @ 6.2, 100 @ 6.
	ladder1.set_unmatched_at(range.price_to_nearest_index(6.2), 50);
	sim->add_bet(123, 6.2, 50, false);
	ladder1.clear_unmatched_at(range.price_to_nearest_index(6.2));
	ladder1.set_unmatched_at(range.price_to_nearest_index(6), 100);
	sim->add_bet(123, 6, 100, false);
	ladder1.clear_unmatched_at(range.price_to_nearest_index(6));

	EXPECT_EQ(sim->bets().size(), 4);

	// Now hedge @ unspecified price (should resolve to 6.4).
	ladder1.set_unmatched_at(range.price_to_nearest_index(6.4), 1);
	EXPECT_TRUE(sim->hedge(123));
	ASSERT_EQ(sim->bets().size(), 5);
	ladder1.clear_unmatched_at(range.price_to_nearest_index(6.4));
	janus::bet& bet = sim->bets()[4];
	EXPECT_EQ(bet.runner_id(), 123);
	EXPECT_FALSE(bet.is_back());
	EXPECT_DOUBLE_EQ(bet.price(), 6.4);
	EXPECT_DOUBLE_EQ(round_2dp(bet.stake()), 101.56);
	EXPECT_TRUE(bet.is_complete());

	// A further hedge should fail.
	ladder1.set_unmatched_at(range.price_to_nearest_index(5.8), 61);
	EXPECT_FALSE(sim->hedge(123, 5.8));
	EXPECT_EQ(sim->bets().size(), 5);
}

// Test that a bet at a higher price but unmatched works as expected.
TEST(sim_test, hedge_unmatched_back)
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

	// Lays: 50 @ 6.2, 100 @ 6.
	ladder1.set_unmatched_at(range.price_to_nearest_index(6.2), 50);
	sim->add_bet(123, 6.2, 50, false);
	ladder1.clear_unmatched_at(range.price_to_nearest_index(6.2));
	ladder1.set_unmatched_at(range.price_to_nearest_index(6), 100);
	sim->add_bet(123, 6, 100, false);
	ladder1.clear_unmatched_at(range.price_to_nearest_index(6));

	EXPECT_EQ(sim->bets().size(), 4);

	// Now hedge @ 8.
	EXPECT_TRUE(sim->hedge(123, 8));
	ASSERT_EQ(sim->bets().size(), 5);
	janus::bet& bet = sim->bets()[4];
	EXPECT_EQ(bet.runner_id(), 123);
	EXPECT_TRUE(bet.is_back());
	EXPECT_DOUBLE_EQ(bet.price(), 8);
	EXPECT_DOUBLE_EQ(round_2dp(bet.stake()), 8.75);
}

// Test that hedging all runners works correctly.
TEST(sim_test, hedge_all)
{
	auto run = [&](uint64_t winner_index) {
		janus::betfair::price_range range;

		janus::betfair::market market1(123456);

		janus::betfair::runner& runner1 = market1.add_runner(123);
		janus::betfair::ladder& ladder1 = runner1.ladder();

		janus::betfair::runner& runner2 = market1.add_runner(456);
		janus::betfair::ladder& ladder2 = runner2.ladder();

		janus::betfair::runner& runner3 = market1.add_runner(789);
		janus::betfair::ladder& ladder3 = runner3.ladder();

		// An empty market being hedged should result in nothing.
		auto sim = std::make_unique<janus::sim>(range, market1);
		EXPECT_FALSE(sim->hedge());
		EXPECT_EQ(sim->bets().size(), 0);

		// Lay 300 @ 6.4, add inventory for back at 3.6.
		ladder1.set_unmatched_at(range.price_to_nearest_index(6.4), 300);
		sim->add_bet(123, 6.4, 300, false);
		ladder1.clear_unmatched_at(range.price_to_nearest_index(6.4));
		// Add inventory to hedge with.
		ladder1.set_unmatched_at(range.price_to_nearest_index(3.6), -300);

		// Back 150 @ 5.8, add inventory for lay at 5.2.
		ladder2.set_unmatched_at(range.price_to_nearest_index(5.8), -150);
		sim->add_bet(456, 5.8, 150, true);
		ladder2.clear_unmatched_at(range.price_to_nearest_index(5.8));
		// Add inventory to hedge with.
		ladder2.set_unmatched_at(range.price_to_nearest_index(5.2), 150);

		// Back 10 @ 2.2, add inventory for lay at 9.
		ladder3.set_unmatched_at(range.price_to_nearest_index(2.2), -10);
		sim->add_bet(789, 2.2, 10, true);
		ladder3.clear_unmatched_at(range.price_to_nearest_index(2.2));
		// Add inventory to hedge with.
		ladder3.set_unmatched_at(range.price_to_nearest_index(9), 10);

		EXPECT_EQ(sim->bets().size(), 3);

		EXPECT_TRUE(sim->hedge());
		ASSERT_EQ(sim->bets().size(), 6);

		// Loss-making hedge.
		janus::bet& hedge1 = sim->bets()[3];
		EXPECT_TRUE(hedge1.is_back());
		EXPECT_DOUBLE_EQ(hedge1.price(), 3.6);
		EXPECT_DOUBLE_EQ(round_2dp(hedge1.stake()), 533.33);

		// Profitable hedge.
		janus::bet& hedge2 = sim->bets()[4];
		EXPECT_FALSE(hedge2.is_back());
		EXPECT_DOUBLE_EQ(hedge2.price(), 5.2);
		EXPECT_DOUBLE_EQ(round_2dp(hedge2.stake()), 167.31);

		// Loss-making hedge.
		janus::bet& hedge3 = sim->bets()[5];
		EXPECT_FALSE(hedge3.is_back());
		EXPECT_DOUBLE_EQ(hedge3.price(), 9);
		EXPECT_DOUBLE_EQ(round_2dp(hedge3.stake()), 2.44);

		switch (winner_index) {
		case 0:
			runner1.set_won();
			break;
		case 1:
			runner2.set_won();
			break;
		case 2:
			runner3.set_won();
			break;
		default:
			EXPECT_TRUE(false) << "Can't happen!";
			return;
		}

		EXPECT_DOUBLE_EQ(round_2dp(sim->pl()), -223.58);
	};

	// No matter which runner won we should be hedged so the outcome should
	// be the same.
	run(0);
	run(1);
	run(2);
}

// Test that we correctly handle bets at inplay.
TEST(sim_test, handle_inplay_bets)
{
	janus::betfair::price_range range;

	janus::betfair::market market1(123456);
	market1.add_runner(123);

	auto sim = std::make_unique<janus::sim>(range, market1);

	janus::bet* lapse_bet = sim->add_bet(123, 6.4, 1000, true, janus::bet_persist_type::LAPSE);
	janus::bet* persist_bet =
		sim->add_bet(123, 6.8, 1000, true, janus::bet_persist_type::PERSIST);

	EXPECT_EQ(lapse_bet->persist(), janus::bet_persist_type::LAPSE);
	EXPECT_DOUBLE_EQ(lapse_bet->unmatched(), 1000);

	EXPECT_EQ(persist_bet->persist(), janus::bet_persist_type::PERSIST);
	EXPECT_DOUBLE_EQ(persist_bet->unmatched(), 1000);

	// MARKET_ON_CLOSE not implemented in the sim yet.

	market1.set_inplay(true);
	sim->update();

	EXPECT_DOUBLE_EQ(lapse_bet->unmatched(), 0);
	EXPECT_TRUE((lapse_bet->flags() & janus::bet_flags::CANCELLED) ==
		    janus::bet_flags::CANCELLED);
	EXPECT_DOUBLE_EQ(persist_bet->unmatched(), 1000);
}
} // namespace
