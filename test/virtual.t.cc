#include "janus.hh"
#include "test_util.hh"

#include <cstdint>
#include <gtest/gtest.h>
#include <vector>

namespace
{
// Test that calc_virtual_bets() correctly generates virtual bets from raw
// ladders.
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

	EXPECT_EQ(atb_bets_d.size(), 5);

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

	// We generate invalid prices but these are processed and scaled
	// accordingly in gen_virt_ladder().
	EXPECT_DOUBLE_EQ(round_2dp(atb_bets_d[4].first), 1);
	EXPECT_DOUBLE_EQ(round_2dp(atb_bets_d[4].second), 1773.95);

	// These values are not cited at all by the page but having tested the
	// algorithm against real data I am confident they are correct, so test
	// them to avoid regressions.

	auto atb_bets_n = janus::betfair::calc_virtual_bets(range, true, {ladder_c, ladder_d});

	EXPECT_EQ(atb_bets_n.size(), 5);

	EXPECT_DOUBLE_EQ(round_2dp(atb_bets_n[0].first), 1.76);
	EXPECT_DOUBLE_EQ(round_2dp(atb_bets_n[0].second), 56.67);

	EXPECT_DOUBLE_EQ(round_2dp(atb_bets_n[1].first), 1.55);
	EXPECT_DOUBLE_EQ(round_2dp(atb_bets_n[1].second), 226.33);

	EXPECT_DOUBLE_EQ(round_2dp(atb_bets_n[2].first), 1.08);
	EXPECT_DOUBLE_EQ(round_2dp(atb_bets_n[2].second), 186);

	EXPECT_DOUBLE_EQ(round_2dp(atb_bets_n[3].first), 1.02);
	EXPECT_DOUBLE_EQ(round_2dp(atb_bets_n[3].second), 1909.05);

	// We generate invalid prices but these are processed and scaled
	// accordingly in gen_virt_ladder().
	EXPECT_DOUBLE_EQ(round_2dp(atb_bets_n[4].first), 1);
	EXPECT_DOUBLE_EQ(round_2dp(atb_bets_n[4].second), 49.9);

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

// Test that gen_virt_ladder() correctly generates a virtual ladder with all
// expected virtual and non-virtual unmatched volume.
TEST(virtual_test, gen_virt_ladder)
{
	// Real raw ATL/ATB data from market 167638247 at 2020-01-20D14:00:00Z.

	janus::betfair::ladder real1 = {
		{214, -53.28}, {212, -8.71},    {210, -24},     {208, -334.87},   {206, -32.94},
		{204, -23.15}, {202, -144.82},  {200, -211.34}, {199, -50},       {198, -134.45},
		{193, -6.08},  {191, -47},      {190, -9.55},   {189, -5},        {185, -1.98},
		{184, -5},     {180, -2},       {168, -15.28},  {163, -214.28},   {162, -0.5},
		{160, -50},    {155, -20},      {150, -100},    {123, -0.02},     {120, -19.09},
		{111, -17.93}, {110, -1193.86}, {108, -400},    {106, -44.62},    {105, -26.4},
		{104, -12},    {103, -1925.92}, {102, -5600.2}, {101, -46058.73},

		{222, 59.09},  {224, 18},       {230, 0.2},     {232, 108},       {236, 10},
		{240, 5},      {250, 4},        {258, 4},       {262, 40},        {264, 15},
		{280, 5},      {298, 4.22},     {305, 2.01},    {960, 2.1},       {1000, 1.71},
		{3400, 0.77},  {8000, 0.01},    {10000, 2},     {15000, 0.6},     {39000, 0.02},
		{80000, 0.01}, {90000, 0.01},   {91000, 1.14},  {95000, 0.01},    {100000, 26.96},
	};

	janus::betfair::ladder real2 = {
		{184, -16.13},   {183, -5},       {182, -39.96},   {181, -2},        {180, -36},
		{179, -2},       {178, -440.69},  {177, -25.82},   {175, -3.58},     {173, -193.79},
		{170, -4},       {167, -11},      {163, -5},       {162, -0.5},      {160, -34.12},
		{159, -1.2},     {158, -1.44},    {150, -2},       {140, -337.5},    {123, -0.02},
		{111, -17.93},   {110, -32},      {108, -400},     {106, -88.62},    {105, -26.4},
		{104, -12},      {103, -1901.19}, {102, -5601.26}, {101, -43292.92},

		{188, 45.7},     {189, 21.86},    {190, 6},        {194, 129},       {195, 7.12},
		{198, 12.32},    {200, 42.22},    {202, 135.79},   {204, 50},        {210, 0.2},
		{212, 11},       {218, 69},       {222, 15},       {232, 1.99},      {248, 1.99},
		{260, 4.1},      {280, 5},        {600, 1.71},     {1000, 1.71},     {3400, 0.77},
		{8000, 0.01},    {10000, 2},      {15000, 0.6},    {39000, 0.02},    {95000, 0.01},
		{100000, 34.19},
	};

	janus::betfair::price_range range;

	auto virt_ladder1 = gen_virt_ladder(range, real1, {real2});

	uint64_t price_indexes[10];
	double vols[10];

	uint64_t count = virt_ladder1.best_atl(10, price_indexes, vols);
	EXPECT_EQ(count, 10);

	// Real BDATL data for ladder 1:

	EXPECT_EQ(price_indexes[0], range.pricex100_to_index(220));
	EXPECT_DOUBLE_EQ(round_2dp(vols[0]), 13.49);

	EXPECT_EQ(price_indexes[1], range.pricex100_to_index(222));
	EXPECT_DOUBLE_EQ(round_2dp(vols[1]), 95.97);

	EXPECT_EQ(price_indexes[2], range.pricex100_to_index(224));
	EXPECT_DOUBLE_EQ(round_2dp(vols[2]), 19.62);

	EXPECT_EQ(price_indexes[3], range.pricex100_to_index(226));
	EXPECT_DOUBLE_EQ(round_2dp(vols[3]), 28.67);

	EXPECT_EQ(price_indexes[4], range.pricex100_to_index(228));
	EXPECT_DOUBLE_EQ(round_2dp(vols[4]), 1.57);

	EXPECT_EQ(price_indexes[5], range.pricex100_to_index(230));
	EXPECT_DOUBLE_EQ(round_2dp(vols[5]), 361.13);

	EXPECT_EQ(price_indexes[6], range.pricex100_to_index(232));
	EXPECT_DOUBLE_EQ(round_2dp(vols[6]), 108);

	EXPECT_EQ(price_indexes[7], range.pricex100_to_index(234));
	EXPECT_DOUBLE_EQ(round_2dp(vols[7]), 2.68);

	EXPECT_EQ(price_indexes[8], range.pricex100_to_index(236));
	EXPECT_DOUBLE_EQ(round_2dp(vols[8]), 10);

	EXPECT_EQ(price_indexes[9], range.pricex100_to_index(238));
	EXPECT_DOUBLE_EQ(round_2dp(vols[9]), 140.86);

	// Real BDATB data for ladder 1:

	count = virt_ladder1.best_atb(10, price_indexes, vols);
	EXPECT_EQ(count, 10);

	EXPECT_EQ(price_indexes[0], range.pricex100_to_index(214));
	EXPECT_DOUBLE_EQ(round_2dp(vols[0]), 53.28);

	EXPECT_EQ(price_indexes[1], range.pricex100_to_index(212));
	EXPECT_DOUBLE_EQ(round_2dp(vols[1]), 68.72);

	EXPECT_EQ(price_indexes[2], range.pricex100_to_index(210));
	EXPECT_DOUBLE_EQ(round_2dp(vols[2]), 29.43);

	EXPECT_EQ(price_indexes[3], range.pricex100_to_index(208));
	EXPECT_DOUBLE_EQ(round_2dp(vols[3]), 334.87);

	EXPECT_EQ(price_indexes[4], range.pricex100_to_index(206));
	// Actual data is 154.42, but rounding error has put us out by 1p. Not a
	// huge issue.
	EXPECT_DOUBLE_EQ(round_2dp(vols[4]), 154.43);

	EXPECT_EQ(price_indexes[5], range.pricex100_to_index(204));
	EXPECT_DOUBLE_EQ(round_2dp(vols[5]), 29.96);

	EXPECT_EQ(price_indexes[6], range.pricex100_to_index(202));
	EXPECT_DOUBLE_EQ(round_2dp(vols[6]), 156.9);

	EXPECT_EQ(price_indexes[7], range.pricex100_to_index(200));
	EXPECT_DOUBLE_EQ(round_2dp(vols[7]), 253.56);

	EXPECT_EQ(price_indexes[8], range.pricex100_to_index(199));
	EXPECT_DOUBLE_EQ(round_2dp(vols[8]), 50);

	EXPECT_EQ(price_indexes[9], range.pricex100_to_index(198));
	EXPECT_DOUBLE_EQ(round_2dp(vols[9]), 272.98);

	auto virt_ladder2 = gen_virt_ladder(range, real2, {real1});

	count = virt_ladder2.best_atl(10, price_indexes, vols);
	EXPECT_EQ(count, 10);

	// Real BDATL data for ladder 2:

	EXPECT_EQ(price_indexes[0], range.pricex100_to_index(188));
	// Actual data is 106.34.
	EXPECT_DOUBLE_EQ(round_2dp(vols[0]), 106.35);

	EXPECT_EQ(price_indexes[1], range.pricex100_to_index(189));
	EXPECT_DOUBLE_EQ(round_2dp(vols[1]), 21.86);

	EXPECT_EQ(price_indexes[2], range.pricex100_to_index(190));
	// Actual data is 15.71.
	EXPECT_DOUBLE_EQ(round_2dp(vols[2]), 15.72);

	EXPECT_EQ(price_indexes[3], range.pricex100_to_index(191));
	EXPECT_DOUBLE_EQ(round_2dp(vols[3]), 26.39);

	EXPECT_EQ(price_indexes[4], range.pricex100_to_index(193));
	// Actual data is 360.89.
	EXPECT_DOUBLE_EQ(round_2dp(vols[4]), 360.90);

	EXPECT_EQ(price_indexes[5], range.pricex100_to_index(194));
	EXPECT_DOUBLE_EQ(round_2dp(vols[5]), 129);

	EXPECT_EQ(price_indexes[6], range.pricex100_to_index(195));
	EXPECT_DOUBLE_EQ(round_2dp(vols[6]), 41.92);

	EXPECT_EQ(price_indexes[7], range.pricex100_to_index(197));
	EXPECT_DOUBLE_EQ(round_2dp(vols[7]), 23.97);

	EXPECT_EQ(price_indexes[8], range.pricex100_to_index(198));
	EXPECT_DOUBLE_EQ(round_2dp(vols[8]), 12.32);

	EXPECT_EQ(price_indexes[9], range.pricex100_to_index(199));
	EXPECT_DOUBLE_EQ(round_2dp(vols[9]), 147);

	// Real BDATB data for ladder 2:

	count = virt_ladder2.best_atb(10, price_indexes, vols);
	EXPECT_EQ(count, 10);

	EXPECT_EQ(price_indexes[0], range.pricex100_to_index(184));
	EXPECT_DOUBLE_EQ(round_2dp(vols[0]), 16.13);

	EXPECT_EQ(price_indexes[1], range.pricex100_to_index(183));
	EXPECT_DOUBLE_EQ(round_2dp(vols[1]), 5);

	EXPECT_EQ(price_indexes[2], range.pricex100_to_index(182));
	EXPECT_DOUBLE_EQ(round_2dp(vols[2]), 39.96);

	EXPECT_EQ(price_indexes[3], range.pricex100_to_index(181));
	EXPECT_DOUBLE_EQ(round_2dp(vols[3]), 74.48);

	EXPECT_EQ(price_indexes[4], range.pricex100_to_index(180));
	EXPECT_DOUBLE_EQ(round_2dp(vols[4]), 58.4);

	EXPECT_EQ(price_indexes[5], range.pricex100_to_index(179));
	EXPECT_DOUBLE_EQ(round_2dp(vols[5]), 2);

	EXPECT_EQ(price_indexes[6], range.pricex100_to_index(178));
	EXPECT_DOUBLE_EQ(round_2dp(vols[6]), 440.69);

	EXPECT_EQ(price_indexes[7], range.pricex100_to_index(177));
	EXPECT_DOUBLE_EQ(round_2dp(vols[7]), 25.82);

	EXPECT_EQ(price_indexes[8], range.pricex100_to_index(175));
	EXPECT_DOUBLE_EQ(round_2dp(vols[8]), 147.02);

	EXPECT_EQ(price_indexes[9], range.pricex100_to_index(173));
	EXPECT_DOUBLE_EQ(round_2dp(vols[9]), 207.43);
}
} // namespace
