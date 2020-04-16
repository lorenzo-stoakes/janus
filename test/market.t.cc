#include "janus.hh"

#include <gtest/gtest.h>
#include <stdexcept>

namespace
{
// Test basic market functionality works correctly (getters/setters, ctor).
TEST(market_test, basic)
{
	janus::betfair::market market1(123456);

	EXPECT_EQ(market1.id(), 123456);
	EXPECT_EQ(market1.state(), janus::betfair::market_state::OPEN);
	EXPECT_EQ(market1.traded_vol(), 0);
	EXPECT_EQ(market1.num_runners(), 0);

	market1.set_traded_vol(123.456);
	EXPECT_DOUBLE_EQ(market1.traded_vol(), 123.456);

	market1.set_state(janus::betfair::market_state::SUSPENDED);
	EXPECT_EQ(market1.state(), janus::betfair::market_state::SUSPENDED);
	market1.set_state(janus::betfair::market_state::OPEN);
	EXPECT_EQ(market1.state(), janus::betfair::market_state::OPEN);
	market1.set_state(janus::betfair::market_state::CLOSED);
	EXPECT_EQ(market1.state(), janus::betfair::market_state::CLOSED);

	// Once marked closed we cannot change state again.
	EXPECT_THROW(market1.set_state(janus::betfair::market_state::OPEN), std::runtime_error);

	// Can we add, find and retrieve a runner?
	janus::betfair::market market2(123456);
	EXPECT_EQ(market2.num_runners(), 0);
	EXPECT_FALSE(market2.contains_runner(123));
	EXPECT_EQ(market2.find_runner(123), nullptr);
	market2.add_runner(456);
	EXPECT_EQ(market2.num_runners(), 1);
	EXPECT_TRUE(market2.contains_runner(456));
	EXPECT_EQ(market2.find_runner(456), &market2[0]);

	market2.add_runner(123);
	EXPECT_EQ(market2.num_runners(), 2);
	EXPECT_TRUE(market2.contains_runner(123));
	EXPECT_TRUE(market2.contains_runner(456));
	EXPECT_FALSE(market2.contains_runner(999));

	EXPECT_EQ(market2.last_timestamp(), 0);
	market2.set_last_timestamp(12345678);
	EXPECT_EQ(market2.last_timestamp(), 12345678);
}

// Test that .clear_state() correctly resets MUTABLE state in a marketbut leaves
// the mutable state alone.
TEST(market_test, clear_state)
{
	// Immutable state.
	janus::betfair::market market(123456);
	market.add_runner(456);
	market.set_state(janus::betfair::market_state::SUSPENDED);
	EXPECT_EQ(market.id(), 123456);
	EXPECT_EQ(market.num_runners(), 1);
	EXPECT_EQ(market.state(), janus::betfair::market_state::SUSPENDED);

	// Mutable state.
	market.set_traded_vol(456.789);
	EXPECT_DOUBLE_EQ(market.traded_vol(), 456.789);

	market.clear_state();

	// Immutable.
	EXPECT_EQ(market.id(), 123456);
	EXPECT_EQ(market.num_runners(), 1);
	EXPECT_EQ(market.state(), janus::betfair::market_state::SUSPENDED);

	// Mutable.
	EXPECT_DOUBLE_EQ(market.traded_vol(), 0);
}
} // namespace
