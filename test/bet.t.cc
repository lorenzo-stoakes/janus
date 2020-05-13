#include "janus.hh"

#include <gtest/gtest.h>

namespace
{
// Test that basic bet functionality works correctly.
TEST(bet_test, basic)
{
	// Assert some basic characteristics.
	janus::bet bet1(1234, 1.21, 1000, true);
	EXPECT_EQ(bet1.flags(), janus::bet_flags::DEFAULT);
	EXPECT_EQ(bet1.runner_id(), 1234);
	EXPECT_DOUBLE_EQ(bet1.price(), 1.21);
	EXPECT_DOUBLE_EQ(bet1.orig_price(), 1.21);
	EXPECT_DOUBLE_EQ(bet1.stake(), 1000);
	EXPECT_DOUBLE_EQ(bet1.orig_stake(), 1000);
	EXPECT_TRUE(bet1.is_back());
	EXPECT_DOUBLE_EQ(bet1.matched(), 0);
	EXPECT_DOUBLE_EQ(bet1.unmatched(), 1000);
	EXPECT_FALSE(bet1.is_matched());
	EXPECT_FALSE(bet1.is_cancelled());
	EXPECT_DOUBLE_EQ(bet1.pl(true), 0);
	EXPECT_DOUBLE_EQ(bet1.pl(false), 0);
	EXPECT_EQ(bet1.bet_id(), 0);

	// Match 500.
	bet1.match(500);
	EXPECT_DOUBLE_EQ(bet1.matched(), 500);
	EXPECT_DOUBLE_EQ(bet1.unmatched(), 500);
	EXPECT_FALSE(bet1.is_matched());
	EXPECT_FALSE(bet1.is_cancelled());
	EXPECT_DOUBLE_EQ(bet1.pl(true), 105);
	EXPECT_DOUBLE_EQ(bet1.pl(false), -500);

	// Cancel rest.
	bet1.cancel();
	EXPECT_DOUBLE_EQ(bet1.matched(), 500);
	EXPECT_DOUBLE_EQ(bet1.unmatched(), 0);
	EXPECT_TRUE(bet1.is_matched());
	EXPECT_EQ(bet1.flags(), janus::bet_flags::CANCELLED);
	EXPECT_DOUBLE_EQ(bet1.pl(true), 105);
	EXPECT_DOUBLE_EQ(bet1.pl(false), -500);

	// Void bet.
	bet1.void_bet();
	EXPECT_DOUBLE_EQ(bet1.matched(), 0);
	EXPECT_DOUBLE_EQ(bet1.unmatched(), 0);
	EXPECT_FALSE(bet1.is_matched());
	EXPECT_EQ(bet1.flags(), janus::bet_flags::CANCELLED | janus::bet_flags::VOIDED);
	EXPECT_DOUBLE_EQ(bet1.pl(true), 0);
	EXPECT_DOUBLE_EQ(bet1.pl(false), 0);
	// Further operations should be no-ops.
	bet1.match(100000);
	bet1.cancel();
	EXPECT_DOUBLE_EQ(bet1.matched(), 0);
	EXPECT_DOUBLE_EQ(bet1.unmatched(), 0);
	EXPECT_FALSE(bet1.is_matched());
	// Setting bet ID on a voided bet should be a no-op.
	bet1.set_bet_id(999);
	EXPECT_EQ(bet1.flags(), janus::bet_flags::CANCELLED | janus::bet_flags::VOIDED);
	EXPECT_EQ(bet1.bet_id(), 0);

	// Lay bet.
	janus::bet bet2(1234, 2.4, 1000, false);
	EXPECT_EQ(bet2.flags(), janus::bet_flags::DEFAULT);
	EXPECT_EQ(bet2.runner_id(), 1234);
	EXPECT_DOUBLE_EQ(bet2.price(), 2.4);
	EXPECT_DOUBLE_EQ(bet2.orig_price(), 2.4);
	EXPECT_DOUBLE_EQ(bet2.stake(), 1000);
	EXPECT_DOUBLE_EQ(bet2.orig_stake(), 1000);
	EXPECT_FALSE(bet2.is_back());
	EXPECT_DOUBLE_EQ(bet2.matched(), 0);
	EXPECT_DOUBLE_EQ(bet2.unmatched(), 1000);
	EXPECT_FALSE(bet2.is_matched());
	EXPECT_FALSE(bet2.is_cancelled());
	EXPECT_DOUBLE_EQ(bet2.pl(true), 0);
	EXPECT_DOUBLE_EQ(bet2.pl(false), 0);
	EXPECT_EQ(bet2.bet_id(), 0);

	// Match 200.
	bet2.match(200);
	EXPECT_DOUBLE_EQ(bet2.matched(), 200);
	EXPECT_DOUBLE_EQ(bet2.unmatched(), 800);
	EXPECT_FALSE(bet2.is_matched());
	EXPECT_FALSE(bet2.is_cancelled());
	EXPECT_DOUBLE_EQ(bet2.pl(true), -280);
	EXPECT_DOUBLE_EQ(bet2.pl(false), 200);

	// Apply adjustment factor.
	bet2.apply_adj_factor(5);
	EXPECT_EQ(bet2.flags(), janus::bet_flags::REDUCED);
	EXPECT_DOUBLE_EQ(bet2.price(), 2.28);
	EXPECT_DOUBLE_EQ(bet2.orig_price(), 2.4);
	EXPECT_DOUBLE_EQ(bet2.pl(true), -256);
	EXPECT_DOUBLE_EQ(bet2.pl(false), 200);

	// Test that .is_cancelled() works correctly.
	janus::bet bet3(1234, 1.21, 1000, true);
	EXPECT_FALSE(bet3.is_cancelled());
	bet3.cancel();
	EXPECT_TRUE(bet3.is_cancelled());

	// Set bet ID.
	EXPECT_EQ(bet3.bet_id(), 0);
	bet3.set_bet_id(999);
	EXPECT_EQ(bet3.bet_id(), 999);
	EXPECT_EQ(bet3.flags(), janus::bet_flags::ACKED | janus::bet_flags::CANCELLED);

	// Adjustment factor below 2.5% should do nothing.
	bet3.apply_adj_factor(2.1);
	EXPECT_DOUBLE_EQ(bet2.price(), 2.28);
	EXPECT_DOUBLE_EQ(bet3.price(), 1.21);
	EXPECT_EQ(bet3.flags(), janus::bet_flags::ACKED | janus::bet_flags::CANCELLED);

	// Sim bet.
	janus::bet bet4(1234, 1.21, 1000, false, true);
	EXPECT_EQ(bet4.flags(), janus::bet_flags::SIM);
	bet4.set_target_matched(123);
	EXPECT_EQ(bet4.target_matched(), 123);
}
} // namespace
