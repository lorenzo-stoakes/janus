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
} // namespace
