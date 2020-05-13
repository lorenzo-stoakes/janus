#include "janus.hh"

#include <gtest/gtest.h>

namespace
{
// Test that deq works correctly.
TEST(util_test, deq)
{
	EXPECT_TRUE(janus::deq(0, 0));
	EXPECT_TRUE(janus::deq(1.23, 1.23));
	EXPECT_FALSE(janus::deq(-1, 1));
	EXPECT_TRUE(janus::deq(1, 1 + 1e-16));
}

// Test that dz works correctly.
TEST(util_test, dz)
{
	EXPECT_TRUE(janus::dz(0));
	EXPECT_TRUE(janus::dz(1e-16));
	EXPECT_FALSE(janus::dz(1e-5));
}
} // namespace
