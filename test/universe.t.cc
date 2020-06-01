#include "janus.hh"

#include <gtest/gtest.h>
#include <memory>
#include <stdexcept>

namespace
{
// Test basic functionality of universe.
TEST(universe_test, basic)
{
	auto ptr = std::make_unique<janus::betfair::universe<50>>();
	auto& universe = *ptr;

	EXPECT_EQ(universe.num_markets(), 0);

	// Can add and access market.
	janus::betfair::market& market = universe.add_market(123);
	EXPECT_EQ(market.id(), 123);
	EXPECT_EQ(universe.num_markets(), 1);
	EXPECT_EQ(universe[123].id(), 123);
	EXPECT_EQ(universe.markets()[0].id(), 123);

	// Can find what we just inserted, but not what doesn't exist.
	EXPECT_NE(universe.find_market(123), nullptr);
	EXPECT_EQ(universe.find_market(456), nullptr);
	EXPECT_TRUE(universe.contains_market(123));
	EXPECT_FALSE(universe.contains_market(456));

	EXPECT_EQ(universe.last_timestamp(), 0);
	universe.set_last_timestamp(12345678);
	EXPECT_EQ(universe.last_timestamp(), 12345678);
}

// Test that we can apply updates to a universe correctly.
TEST(universe_test, apply_update)
{
	auto ptr = std::make_unique<janus::betfair::universe<2>>();
	auto& universe = *ptr;

	// We haven't sent a market ID yet so the universe doesn't know what to
	// do with an arbitrary update.
	EXPECT_THROW(universe.apply_update(janus::make_market_clear_update()),
		     janus::universe_apply_error);

	// Add a market.
	EXPECT_EQ(universe.num_markets(), 0);
	EXPECT_EQ(universe.num_updates(), 0);
	EXPECT_EQ(universe.last_market(), nullptr);
	universe.apply_update(janus::make_market_id_update(123456));
	EXPECT_EQ(universe.num_updates(), 1);
	EXPECT_EQ(universe.num_markets(), 1);
	ASSERT_NE(universe.find_market(123456), nullptr);
	EXPECT_EQ(universe[123456].id(), 123456);
	EXPECT_EQ(universe.last_market()->id(), 123456);

	// We haven't sent a runner ID yet so the universe doesn't know what to
	// do with a runner update.
	EXPECT_THROW(universe.apply_update(janus::make_runner_won_update()),
		     janus::universe_apply_error);

	// Add a runner.
	EXPECT_EQ(universe[123456].num_runners(), 0);
	EXPECT_EQ(universe.last_runner(), nullptr);
	universe.apply_update(janus::make_runner_id_update(789));
	EXPECT_EQ(universe.num_updates(), 2);
	EXPECT_EQ(universe[123456].num_runners(), 1);
	EXPECT_EQ(universe[123456][0].id(), 789);
	EXPECT_EQ(universe.last_runner()->id(), 789);

	// We haven't sent a timestamp yet so the universe doesn't know what to
	// do with an arbitrary update.
	ASSERT_THROW(universe.apply_update(janus::make_runner_won_update()),
		     janus::universe_apply_error);

	// Set the timestamp.
	universe.apply_update(janus::make_timestamp_update(1234567));
	EXPECT_EQ(universe.num_updates(), 3);
	EXPECT_EQ(universe.last_timestamp(), 1234567);
	EXPECT_EQ(universe[123456].last_timestamp(), 1234567);
	// The runner timestamp should NOT yet be updated.
	EXPECT_EQ(universe[123456][0].last_timestamp(), 0);

	// We should now be able to send any update we like.

	// Market traded volume.
	universe.apply_update(janus::make_market_traded_vol_update(123.456));
	EXPECT_EQ(universe.num_updates(), 4);
	EXPECT_DOUBLE_EQ(universe[123456].traded_vol(), 123.456);
	// Now we've updated the market we should see the market timestamp get updated.
	EXPECT_EQ(universe[123456].last_timestamp(), 1234567);
	// But not the runner timestamp.
	EXPECT_EQ(universe[123456][0].last_timestamp(), 0);

	// Now add another market to make sure we set traded volume there instead.
	universe.apply_update(janus::make_market_id_update(999));
	EXPECT_EQ(universe.num_updates(), 5);
	EXPECT_EQ(universe.num_markets(), 2);
	EXPECT_EQ(universe.last_market()->id(), 999);
	// Now we changed market, our last runner should be invalidated.
	EXPECT_EQ(universe.last_runner(), nullptr);
	ASSERT_THROW(universe.apply_update(janus::make_runner_won_update()),
		     janus::universe_apply_error);
	// Set market traded volume on other market.
	universe.apply_update(janus::make_market_traded_vol_update(789.99));
	EXPECT_EQ(universe.num_updates(), 6);
	EXPECT_DOUBLE_EQ(universe.last_market()->traded_vol(), 789.99);
	EXPECT_DOUBLE_EQ(universe[123456].traded_vol(), 123.456);
	EXPECT_DOUBLE_EQ(universe[999].traded_vol(), 789.99);

	// Switch back to our previous market and runner.
	universe.apply_update(janus::make_market_id_update(123456));
	EXPECT_EQ(universe.num_updates(), 7);
	// We shouldn't have created a new market.
	EXPECT_EQ(universe.num_markets(), 2);
	// We should be back to our original market traded volume.
	EXPECT_DOUBLE_EQ(universe.last_market()->traded_vol(), 123.456);
	universe.apply_update(janus::make_runner_id_update(789));
	EXPECT_EQ(universe.num_updates(), 8);
	// We shouldn't have created a new runner.
	EXPECT_EQ(universe[123456].num_runners(), 1);

	// Mark market suspended.
	universe.apply_update(janus::make_market_suspend_update());
	EXPECT_EQ(universe.num_updates(), 9);
	EXPECT_EQ(universe.last_market()->state(), janus::betfair::market_state::SUSPENDED);

	// Mark market open.
	universe.apply_update(janus::make_market_open_update());
	EXPECT_EQ(universe.num_updates(), 10);
	EXPECT_EQ(universe.last_market()->state(), janus::betfair::market_state::OPEN);

	// Mark market inplay.
	universe.apply_update(janus::make_market_inplay_update());
	EXPECT_EQ(universe.num_updates(), 11);
	EXPECT_TRUE(universe.last_market()->inplay());

	// Now market clear.
	EXPECT_DOUBLE_EQ(universe.last_market()->traded_vol(), 123.456);
	universe.apply_update(janus::make_market_clear_update());
	EXPECT_EQ(universe.num_updates(), 12);
	// We should have cleared this.
	EXPECT_DOUBLE_EQ(universe.last_market()->traded_vol(), 0);
	// Last runner is invalidated on market clear.
	EXPECT_EQ(universe.last_runner(), nullptr);

	// Mark market closed.
	universe.apply_update(janus::make_market_close_update());
	EXPECT_EQ(universe.num_updates(), 13);
	EXPECT_EQ(universe.last_market()->state(), janus::betfair::market_state::CLOSED);

	// Switch to our open market.
	universe.apply_update(janus::make_market_id_update(999));
	EXPECT_EQ(universe.num_updates(), 14);
	EXPECT_EQ(universe.last_market()->num_runners(), 0);

	// Add a runner.
	universe.apply_update(janus::make_runner_id_update(777));
	EXPECT_EQ(universe.num_updates(), 15);
	EXPECT_EQ(universe.last_market()->num_runners(), 1);
	EXPECT_EQ(universe.last_runner()->id(), 777);

	// Set runner traded volume.

	// Runner not updated yet so shouldn't have a timestamp set.
	EXPECT_EQ(universe.last_runner()->last_timestamp(), 0);
	universe.apply_update(janus::make_runner_traded_vol_update(789.12));
	EXPECT_EQ(universe.num_updates(), 16);
	EXPECT_DOUBLE_EQ(universe.last_runner()->traded_vol(), 789.12);
	// Now we've updated a runner we should see its timestamp updated.
	EXPECT_EQ(universe.last_runner()->last_timestamp(), 1234567);

	// Add a new runner and make sure we update that runner not the previous.
	universe.apply_update(janus::make_runner_id_update(654));
	EXPECT_EQ(universe.num_updates(), 17);
	EXPECT_EQ(universe.last_market()->num_runners(), 2);
	universe.apply_update(janus::make_runner_traded_vol_update(111.23));
	EXPECT_EQ(universe.num_updates(), 18);
	EXPECT_DOUBLE_EQ(universe.last_runner()->traded_vol(), 111.23);
	// Ensure previous runner not changed.
	auto* prev_runner = universe.last_market()->find_runner(777);
	EXPECT_DOUBLE_EQ(prev_runner->traded_vol(), 789.12);

	// Set runner LTP.
	universe.apply_update(janus::make_runner_ltp_update(17));
	EXPECT_EQ(universe.num_updates(), 19);
	EXPECT_EQ(universe.last_runner()->ltp(), 17);

	// Set runner SP.
	universe.apply_update(janus::make_runner_sp_update(654.12));
	EXPECT_EQ(universe.num_updates(), 20);
	EXPECT_DOUBLE_EQ(universe.last_runner()->sp(), 654.12);

	// Set runner matched.
	universe.apply_update(janus::make_runner_matched_update(17, 123.45));
	EXPECT_EQ(universe.num_updates(), 21);
	EXPECT_DOUBLE_EQ(universe.last_runner()->ladder().matched(17), 123.45);

	// Set runner unmatched ATL.
	universe.apply_update(janus::make_runner_unmatched_atl_update(12, 3.92));
	EXPECT_EQ(universe.num_updates(), 22);
	EXPECT_DOUBLE_EQ(universe.last_runner()->ladder().unmatched(12), 3.92);

	// Set runner unmatched ATB.
	universe.apply_update(janus::make_runner_unmatched_atb_update(8, 157.9));
	EXPECT_EQ(universe.num_updates(), 23);
	// ATB so negative.
	EXPECT_DOUBLE_EQ(universe.last_runner()->ladder().unmatched(8), -157.9);

	// Clear runner unmatched state.
	universe.apply_update(janus::make_runner_clear_unmatched());
	EXPECT_EQ(universe.num_updates(), 24);
	EXPECT_DOUBLE_EQ(universe.last_runner()->ladder().unmatched(12), 0);
	EXPECT_DOUBLE_EQ(universe.last_runner()->ladder().unmatched(8), 0);

	// Set runner won.
	universe.apply_update(janus::make_runner_won_update());
	EXPECT_EQ(universe.num_updates(), 25);
	EXPECT_EQ(universe.last_runner()->state(), janus::betfair::runner_state::WON);
}

// Test that a universe can receive a timestamp update BEFORE a market ID or
// runner ID update.
TEST(universe_test, timestamp_first)
{
	// No market ID, runner ID.

	auto ptr1 = std::make_unique<janus::betfair::universe<1>>();
	auto& universe1 = *ptr1;

	ASSERT_NO_THROW(universe1.apply_update(janus::make_timestamp_update(1234567)));

	// No runner ID.

	auto ptr2 = std::make_unique<janus::betfair::universe<1>>();
	auto& universe2 = *ptr2;

	universe2.apply_update(janus::make_market_id_update(123456));
	ASSERT_NO_THROW(universe2.apply_update(janus::make_timestamp_update(1234567)));
}

// Ensure that timestamps cannot go backwards.
TEST(universe_test, dont_time_travel)
{
	auto ptr1 = std::make_unique<janus::betfair::universe<1>>();
	auto& universe1 = *ptr1;

	universe1.apply_update(janus::make_timestamp_update(1234567));
	ASSERT_THROW(universe1.apply_update(janus::make_timestamp_update(234567)),
		     janus::universe_apply_error);
}

// Ensure that clearing the universe clears all markets down.
TEST(universe_test, clear)
{
	auto ptr = std::make_unique<janus::betfair::universe<1>>();
	auto& universe = *ptr;
	EXPECT_EQ(universe.num_markets(), 0);
	universe.apply_update(janus::make_market_id_update(123456));
	EXPECT_EQ(universe.num_markets(), 1);
	universe.clear();
	EXPECT_EQ(universe.num_markets(), 0);
	EXPECT_EQ(universe.last_timestamp(), 0);
	EXPECT_EQ(universe.num_updates(), 0);
	EXPECT_EQ(universe.last_market(), nullptr);
	EXPECT_EQ(universe.last_runner(), nullptr);
}
} // namespace
