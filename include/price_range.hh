#pragma once

#include <array>
#include <cmath>
#include <cstdint>

namespace janus::betfair
{
static constexpr uint64_t NUM_PRICES = 350;
static constexpr uint64_t MIN_PRICEX100 = 101;
static constexpr uint64_t MAX_PRICEX100 = 100000;
static constexpr uint64_t INVALID_PRICE_INDEX = static_cast<uint64_t>(-1);
static constexpr uint64_t INVALID_PRICEX100 = static_cast<uint64_t>(-2);

// Represents the valid range of prices (i.e. odds) and provides methods to find
// nearest prices, map between index and price and so on.
class price_range
{
public:
	// Initialise price range, this populates the price map.
	price_range() // NOLINT: We initialise price_map in the ctor body.
	{
		populate_price_map();
	}

	// The [] operator returns pricex100 values as these are more accurate
	// and easier to work with.
	auto operator[](uint64_t i) -> uint64_t
	{
		return index_to_pricex100(i);
	}

	// Obtain pricex100 value at specified price index. Note that the index
	// is NOT checked.
	static auto index_to_pricex100(uint64_t i) -> uint64_t
	{
		return PRICESX100[i];
	}

	// Obtain floating point price value at specified price index. Note that the
	// index is NOT checked.
	static auto index_to_price(uint64_t i) -> double
	{
		return PRICES[i];
	}

	// Obtain the NEAREST price index of the specified pricex100, rounding
	// down, e.g. 627 -> index of 620.
	// Note that the pricex100 is NOT checked, i.e. if it is larger than
	// MAX_PRICEX100 this will buffer overflow.
	auto pricex100_to_nearest_index(uint64_t pricex100) const -> uint64_t
	{
		return _price_map[pricex100];
	}

	// Obtain the PRECISE price index of the specified pricex100, if it is
	// not a valid price then INVALID_PRICE_INDEX is returned.
	auto pricex100_to_index(uint64_t pricex100) const -> uint64_t
	{
		uint64_t i = pricex100_to_nearest_index(pricex100);
		if (i == INVALID_PRICE_INDEX || index_to_pricex100(i) != pricex100)
			return INVALID_PRICE_INDEX;

		return i;
	}

	// Find the nearest pricex100 to the input pricex100, rounding down,
	// e.g. 627 -> 620.
	// Note that the pricex100 is NOT checked, i.e. if it is larger than
	// MAX_PRICEX100 this will buffer overflow.
	// If the price is not valid then INVALID_PRICEX100 is returned.
	auto nearest_pricex100(uint64_t pricex100) const -> uint64_t
	{
		uint64_t i = pricex100_to_nearest_index(pricex100);
		if (i == INVALID_PRICE_INDEX)
			return INVALID_PRICEX100;

		return index_to_pricex100(i);
	}

	// Find the index of the nearest price to the specified price, rounding
	// the price such that 6.19999 is correctly rounded to 6.2, and rounding
	// down such that 6.27 -> index of 6.2.
	// Note that the price is NOT checked, i.e. if it is larger than 1000
	// this will buffer overflow.
	auto price_to_nearest_index(double price) const -> uint64_t
	{
		// Round to price x 10,000, which should help avoid rounding
		// errors, but divide DOWN to x 100 in order that we maintain
		// our bias towards the lower pricex100.
		uint64_t pricex10000 = ::llround(price * 10000.);
		uint64_t pricex100 = pricex10000 / 100;

		return pricex100_to_nearest_index(pricex100);
	}

	// Find the nearest pricex100 to the specified price, rounding the price
	// such that 6.19999 is correctly rounded to 6.2, and rounding down such
	// that 6.27 -> 620.
	// Note that the price is NOT checked, i.e. if it is larger than 1000
	// this will buffer overflow.
	auto price_to_nearest_pricex100(double price) const -> uint64_t
	{
		uint64_t i = price_to_nearest_index(price);
		if (i == INVALID_PRICE_INDEX)
			return INVALID_PRICEX100;

		return index_to_pricex100(i);
	}

private:
	static constexpr std::array<double, NUM_PRICES> PRICES = {
		1.01, 1.02, 1.03, 1.04, 1.05, 1.06, 1.07, 1.08, 1.09, 1.1,  1.11, 1.12, 1.13, 1.14,
		1.15, 1.16, 1.17, 1.18, 1.19, 1.2,  1.21, 1.22, 1.23, 1.24, 1.25, 1.26, 1.27, 1.28,
		1.29, 1.3,  1.31, 1.32, 1.33, 1.34, 1.35, 1.36, 1.37, 1.38, 1.39, 1.4,  1.41, 1.42,
		1.43, 1.44, 1.45, 1.46, 1.47, 1.48, 1.49, 1.5,  1.51, 1.52, 1.53, 1.54, 1.55, 1.56,
		1.57, 1.58, 1.59, 1.6,  1.61, 1.62, 1.63, 1.64, 1.65, 1.66, 1.67, 1.68, 1.69, 1.7,
		1.71, 1.72, 1.73, 1.74, 1.75, 1.76, 1.77, 1.78, 1.79, 1.8,  1.81, 1.82, 1.83, 1.84,
		1.85, 1.86, 1.87, 1.88, 1.89, 1.9,  1.91, 1.92, 1.93, 1.94, 1.95, 1.96, 1.97, 1.98,
		1.99, 2,    2.02, 2.04, 2.06, 2.08, 2.1,  2.12, 2.14, 2.16, 2.18, 2.2,  2.22, 2.24,
		2.26, 2.28, 2.3,  2.32, 2.34, 2.36, 2.38, 2.4,  2.42, 2.44, 2.46, 2.48, 2.5,  2.52,
		2.54, 2.56, 2.58, 2.6,  2.62, 2.64, 2.66, 2.68, 2.7,  2.72, 2.74, 2.76, 2.78, 2.8,
		2.82, 2.84, 2.86, 2.88, 2.9,  2.92, 2.94, 2.96, 2.98, 3,    3.05, 3.1,  3.15, 3.2,
		3.25, 3.3,  3.35, 3.4,  3.45, 3.5,  3.55, 3.6,  3.65, 3.7,  3.75, 3.8,  3.85, 3.9,
		3.95, 4,    4.1,  4.2,  4.3,  4.4,  4.5,  4.6,  4.7,  4.8,  4.9,  5,    5.1,  5.2,
		5.3,  5.4,  5.5,  5.6,  5.7,  5.8,  5.9,  6,    6.2,  6.4,  6.6,  6.8,  7,    7.2,
		7.4,  7.6,  7.8,  8,    8.2,  8.4,  8.6,  8.8,  9,    9.2,  9.4,  9.6,  9.8,  10,
		10.5, 11,   11.5, 12,   12.5, 13,   13.5, 14,   14.5, 15,   15.5, 16,   16.5, 17,
		17.5, 18,   18.5, 19,   19.5, 20,   21,   22,   23,   24,   25,   26,   27,   28,
		29,   30,   32,   34,   36,   38,   40,   42,   44,   46,   48,   50,   55,   60,
		65,   70,   75,   80,   85,   90,   95,   100,  110,  120,  130,  140,  150,  160,
		170,  180,  190,  200,  210,  220,  230,  240,  250,  260,  270,  280,  290,  300,
		310,  320,  330,  340,  350,  360,  370,  380,  390,  400,  410,  420,  430,  440,
		450,  460,  470,  480,  490,  500,  510,  520,  530,  540,  550,  560,  570,  580,
		590,  600,  610,  620,  630,  640,  650,  660,  670,  680,  690,  700,  710,  720,
		730,  740,  750,  760,  770,  780,  790,  800,  810,  820,  830,  840,  850,  860,
		870,  880,  890,  900,  910,  920,  930,  940,  950,  960,  970,  980,  990,  1000,
	};

	static constexpr std::array<uint64_t, NUM_PRICES> PRICESX100 = {
		101,   102,    103,   104,   105,   106,   107,   108,   109,   110,   111,   112,
		113,   114,    115,   116,   117,   118,   119,   120,   121,   122,   123,   124,
		125,   126,    127,   128,   129,   130,   131,   132,   133,   134,   135,   136,
		137,   138,    139,   140,   141,   142,   143,   144,   145,   146,   147,   148,
		149,   150,    151,   152,   153,   154,   155,   156,   157,   158,   159,   160,
		161,   162,    163,   164,   165,   166,   167,   168,   169,   170,   171,   172,
		173,   174,    175,   176,   177,   178,   179,   180,   181,   182,   183,   184,
		185,   186,    187,   188,   189,   190,   191,   192,   193,   194,   195,   196,
		197,   198,    199,   200,   202,   204,   206,   208,   210,   212,   214,   216,
		218,   220,    222,   224,   226,   228,   230,   232,   234,   236,   238,   240,
		242,   244,    246,   248,   250,   252,   254,   256,   258,   260,   262,   264,
		266,   268,    270,   272,   274,   276,   278,   280,   282,   284,   286,   288,
		290,   292,    294,   296,   298,   300,   305,   310,   315,   320,   325,   330,
		335,   340,    345,   350,   355,   360,   365,   370,   375,   380,   385,   390,
		395,   400,    410,   420,   430,   440,   450,   460,   470,   480,   490,   500,
		510,   520,    530,   540,   550,   560,   570,   580,   590,   600,   620,   640,
		660,   680,    700,   720,   740,   760,   780,   800,   820,   840,   860,   880,
		900,   920,    940,   960,   980,   1000,  1050,  1100,  1150,  1200,  1250,  1300,
		1350,  1400,   1450,  1500,  1550,  1600,  1650,  1700,  1750,  1800,  1850,  1900,
		1950,  2000,   2100,  2200,  2300,  2400,  2500,  2600,  2700,  2800,  2900,  3000,
		3200,  3400,   3600,  3800,  4000,  4200,  4400,  4600,  4800,  5000,  5500,  6000,
		6500,  7000,   7500,  8000,  8500,  9000,  9500,  10000, 11000, 12000, 13000, 14000,
		15000, 16000,  17000, 18000, 19000, 20000, 21000, 22000, 23000, 24000, 25000, 26000,
		27000, 28000,  29000, 30000, 31000, 32000, 33000, 34000, 35000, 36000, 37000, 38000,
		39000, 40000,  41000, 42000, 43000, 44000, 45000, 46000, 47000, 48000, 49000, 50000,
		51000, 52000,  53000, 54000, 55000, 56000, 57000, 58000, 59000, 60000, 61000, 62000,
		63000, 64000,  65000, 66000, 67000, 68000, 69000, 70000, 71000, 72000, 73000, 74000,
		75000, 76000,  77000, 78000, 79000, 80000, 81000, 82000, 83000, 84000, 85000, 86000,
		87000, 88000,  89000, 90000, 91000, 92000, 93000, 94000, 95000, 96000, 97000, 98000,
		99000, 100000,
	};

	// Perfect hash mapping from pricex100 to price index. It maps to the
	// nearest price rounding down, e.g. 627 -> 620.
	std::array<uint64_t, MAX_PRICEX100 + 1> _price_map;

	// Populate the price map array.
	void populate_price_map()
	{
		// All values less than the minimum pricex100 (101) are marked
		// as invalid.
		for (uint64_t i = 0; i < MIN_PRICEX100; i++) {
			_price_map[i] = INVALID_PRICE_INDEX;
		}

		// For the rest we map the nearest price less than or equal to
		// the index. This allows for 'fuzzy' matching while still
		// allowing precise matching by comparing
		// PRICESX100[price_index] == pricex100.
		int price_index = 0;
		for (uint64_t i = MIN_PRICEX100; i <= MAX_PRICEX100; i++) {
			if (i > PRICESX100[price_index])
				price_index++;

			_price_map[i] = price_index;
		}
	}
};
} // namespace janus::betfair
