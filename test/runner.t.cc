#include "janus.hh"

#include <gtest/gtest.h>

namespace
{
// Test that basic functionality, i.e. ctor/getters/setters work correctly.
TEST(runner_test, basic)
{
	janus::betfair::runner runner1(123456);
	EXPECT_EQ(runner1.id(), 123456);

	EXPECT_DOUBLE_EQ(runner1.traded_vol(), 0);
	runner1.set_traded_vol(456.789);
	EXPECT_DOUBLE_EQ(runner1.traded_vol(), 456.789);

	EXPECT_EQ(runner1.state(), janus::betfair::runner_state::ACTIVE);
	runner1.set_won();
	EXPECT_EQ(runner1.state(), janus::betfair::runner_state::WON);
	// We should only be able to go to removed if the runner is active.
	EXPECT_THROW(runner1.set_removed(0), std::runtime_error);

	janus::betfair::runner runner2(456789);
	EXPECT_EQ(runner2.id(), 456789);
	EXPECT_EQ(runner2.state(), janus::betfair::runner_state::ACTIVE);
	EXPECT_DOUBLE_EQ(runner2.adj_factor(), 0);
	runner2.set_removed(1.2345);
	EXPECT_EQ(runner2.state(), janus::betfair::runner_state::REMOVED);
	EXPECT_DOUBLE_EQ(runner2.adj_factor(), 1.2345);
	// We should only be able to go to won if the runner is active.
	EXPECT_THROW(runner2.set_won(), std::runtime_error);

	janus::betfair::price_range range;

	janus::betfair::runner runner3(999);
	EXPECT_EQ(runner3.id(), 999);
	runner3.set_ltp(range.pricex100_to_index(620));
	EXPECT_EQ(range[runner3.ltp()], 620);

	janus::betfair::ladder& ladder = runner3.ladder();
	uint64_t price_index = range.pricex100_to_index(620);
	ladder.set_unmatched_at(price_index, 12.345);
	EXPECT_DOUBLE_EQ(ladder[price_index], 12.345);

	// Check SP.
	runner3.set_sp(1.23456);
	EXPECT_DOUBLE_EQ(runner3.sp(), 1.23456);
}
} // namespace
