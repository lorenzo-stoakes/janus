#include "ladder.hh"
#include "price_range.hh"
#include "virtual.hh"

#include <cmath>
#include <cstdint>
#include <gtest/gtest.h>
#include <vector>

namespace
{
// Quick and dirty function to round a number to 2 decimal places.
static double round_2dp(double n)
{
	uint64_t nx100 = ::llround(n * 100.);
	return static_cast<double>(nx100) / 100;
}

// Test that calc_virtual_bets() correctly generates virtual bets from raw ladder
TEST(virtual_test, calc_virtual_bets)
{
	// Using example from
	// https://docs.developer.betfair.com/display/1smk3cen4v3lu3yomq5qye0ni/Additional+Information#AdditionalInformation-VirtualBets

	janus::betfair::ladder ladder_n = {
		{101, -999}, {150, -300}, {200, 120}, {250, 75}, {100000, 2}};

	janus::betfair::ladder ladder_c = {{101, -999}, {240, -250}, {250, -150},
					   {300, 150},  {2000, 10},  {100000, 2}};

	janus::betfair::ladder ladder_d = {{101, -999}, {300, -250}, {500, -150},
					   {1000, 10},  {5000, 50},  {100000, 2}};

	janus::betfair::price_range range;

	// Note that we calculate _using_ ATL values but we _generate_ ATB bets.
	auto atb_bets_d = janus::betfair::calc_virtual_bets(range, true, {ladder_n, ladder_c});

	EXPECT_EQ(atb_bets_d.size(), 4);

	// These are explicitly cited in betfair's example.

	EXPECT_DOUBLE_EQ(atb_bets_d[0].first, 6);
	EXPECT_DOUBLE_EQ(atb_bets_d[0].second, 40);

	EXPECT_DOUBLE_EQ(atb_bets_d[1].first, 3.75);
	EXPECT_DOUBLE_EQ(atb_bets_d[1].second, 50);

	// These (rounded) prices are explicitly cited in betfair's example but
	// the volumes are not:

	EXPECT_DOUBLE_EQ(round_2dp(atb_bets_d[2].first), 1.5);
	EXPECT_DOUBLE_EQ(round_2dp(atb_bets_d[2].second), 14.98);

	EXPECT_DOUBLE_EQ(round_2dp(atb_bets_d[3].first), 1.05);
	EXPECT_DOUBLE_EQ(round_2dp(atb_bets_d[3].second), 189.8);

	// These values are not cited at all by the page but having tested the
	// algorithm against real data I am confident they are correct, so test
	// them to avoid regressions.

	auto atb_bets_n = janus::betfair::calc_virtual_bets(range, true, {ladder_c, ladder_d});

	EXPECT_EQ(atb_bets_n.size(), 4);

	EXPECT_DOUBLE_EQ(round_2dp(atb_bets_n[0].first), 1.76);
	EXPECT_DOUBLE_EQ(round_2dp(atb_bets_n[0].second), 56.67);

	EXPECT_DOUBLE_EQ(round_2dp(atb_bets_n[1].first), 1.55);
	EXPECT_DOUBLE_EQ(round_2dp(atb_bets_n[1].second), 226.33);

	EXPECT_DOUBLE_EQ(round_2dp(atb_bets_n[2].first), 1.08);
	EXPECT_DOUBLE_EQ(round_2dp(atb_bets_n[2].second), 186);

	EXPECT_DOUBLE_EQ(round_2dp(atb_bets_n[3].first), 1.02);
	EXPECT_DOUBLE_EQ(round_2dp(atb_bets_n[3].second), 1909.05);

	auto atb_bets_c = janus::betfair::calc_virtual_bets(range, true, {ladder_n, ladder_d});

	EXPECT_EQ(atb_bets_c.size(), 4);

	EXPECT_DOUBLE_EQ(round_2dp(atb_bets_c[0].first), 2.5);
	EXPECT_DOUBLE_EQ(round_2dp(atb_bets_c[0].second), 40);

	EXPECT_DOUBLE_EQ(round_2dp(atb_bets_c[1].first), 2.08);
	EXPECT_DOUBLE_EQ(round_2dp(atb_bets_c[1].second), 67.2);

	EXPECT_DOUBLE_EQ(round_2dp(atb_bets_c[2].first), 1.72);
	EXPECT_DOUBLE_EQ(round_2dp(atb_bets_c[2].second), 108.75);

	EXPECT_DOUBLE_EQ(round_2dp(atb_bets_c[3].first), 1.02);
	EXPECT_DOUBLE_EQ(round_2dp(atb_bets_c[3].second), 1958);

	// Now for the lay (ATB) example (generating ATL bets).

	janus::betfair::ladder ladder_n2 = {{101, -999}, {150, -200}, {200, -300},
					    {400, 120},  {1500, 75},  {100000, 2}};

	janus::betfair::ladder ladder_c2 = {{101, -999}, {240, -250}, {300, -150},
					    {500, 150},  {2000, 10},  {1000, 2}};

	janus::betfair::ladder ladder_d2 = {{101, -999}, {300, -250}, {500, -150},
					    {1000, 100}, {5000, 50},  {100000, 2}};

	auto atl_bets_d2 = janus::betfair::calc_virtual_bets(range, false, {ladder_n2, ladder_c2});

	EXPECT_EQ(atl_bets_d2.size(), 2);

	// This is the example shown on the betfair page.

	EXPECT_DOUBLE_EQ(atl_bets_d2[0].first, 6);
	EXPECT_DOUBLE_EQ(atl_bets_d2[0].second, 75);

	EXPECT_DOUBLE_EQ(atl_bets_d2[1].first, 12);
	EXPECT_DOUBLE_EQ(atl_bets_d2[1].second, 12.5);

	// Again, these values are not explicitly discussed on the example page
	// but I believe they are correct so including as a regression test:

	auto atl_bets_n2 = janus::betfair::calc_virtual_bets(range, false, {ladder_c2, ladder_d2});

	EXPECT_EQ(atl_bets_n2.size(), 3);

	EXPECT_DOUBLE_EQ(round_2dp(atl_bets_n2[0].first), 2.14);
	EXPECT_DOUBLE_EQ(round_2dp(atl_bets_n2[0].second), 210);

	EXPECT_DOUBLE_EQ(round_2dp(atl_bets_n2[1].first), 2.61);
	EXPECT_DOUBLE_EQ(round_2dp(atl_bets_n2[1].second), 115);

	EXPECT_DOUBLE_EQ(round_2dp(atl_bets_n2[2].first), 4);
	EXPECT_DOUBLE_EQ(round_2dp(atl_bets_n2[2].second), 75);

	auto atl_bets_c2 = janus::betfair::calc_virtual_bets(range, false, {ladder_n2, ladder_d2});

	EXPECT_EQ(atl_bets_c2.size(), 2);

	EXPECT_DOUBLE_EQ(round_2dp(atl_bets_c2[0].first), 3.33);
	EXPECT_DOUBLE_EQ(round_2dp(atl_bets_c2[0].second), 180);

	EXPECT_DOUBLE_EQ(round_2dp(atl_bets_c2[1].first), 7.5);
	EXPECT_DOUBLE_EQ(round_2dp(atl_bets_c2[1].second), 20);
}
} // namespace
