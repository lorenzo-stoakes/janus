#include "janus.hh"

#include <gtest/gtest.h>
#include <memory>

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
} // namespace
