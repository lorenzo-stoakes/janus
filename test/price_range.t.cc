#include "price_range.hh"

#include <gtest/gtest.h>

// Note that by its nature this test involves a lot of duplication. I think this
// is legitimate as we want to be very specific in testing expected price range
// values and the RoI on completely de-duplicating test code (which should
// arguably be as verbose as possible anyway) is not great.

namespace
{
// Test that the .index_to_pricex100() method and [] operator overload correctly
// obtain the expected pricex100's.
TEST(price_range_test, index_to_pricex100)
{
	janus::betfair::price_range range;

	// Test each of the tick ranges one-by-one and ensure that returned
	// values are as expected.

	uint64_t i = 0;

	for (uint64_t pricex100 = 101; pricex100 < 200; pricex100++) {
		ASSERT_EQ(range.index_to_pricex100(i), pricex100);
		ASSERT_EQ(range[i], pricex100);

		i++;
	}

	for (uint64_t pricex100 = 200; pricex100 < 300; pricex100 += 2) {
		ASSERT_EQ(range.index_to_pricex100(i), pricex100);
		ASSERT_EQ(range[i], pricex100);

		i++;
	}

	for (uint64_t pricex100 = 300; pricex100 < 400; pricex100 += 5) {
		ASSERT_EQ(range.index_to_pricex100(i), pricex100);
		ASSERT_EQ(range[i], pricex100);

		i++;
	}

	for (uint64_t pricex100 = 400; pricex100 < 600; pricex100 += 10) {
		ASSERT_EQ(range.index_to_pricex100(i), pricex100);
		ASSERT_EQ(range[i], pricex100);

		i++;
	}

	for (uint64_t pricex100 = 600; pricex100 < 1000; pricex100 += 20) {
		ASSERT_EQ(range.index_to_pricex100(i), pricex100);
		ASSERT_EQ(range[i], pricex100);

		i++;
	}

	for (uint64_t pricex100 = 1000; pricex100 < 2000; pricex100 += 50) {
		ASSERT_EQ(range.index_to_pricex100(i), pricex100);
		ASSERT_EQ(range[i], pricex100);

		i++;
	}

	for (uint64_t pricex100 = 2000; pricex100 < 3000; pricex100 += 100) {
		ASSERT_EQ(range.index_to_pricex100(i), pricex100);
		ASSERT_EQ(range[i], pricex100);

		i++;
	}

	for (uint64_t pricex100 = 3000; pricex100 < 5000; pricex100 += 200) {
		ASSERT_EQ(range.index_to_pricex100(i), pricex100);
		ASSERT_EQ(range[i], pricex100);

		i++;
	}

	for (uint64_t pricex100 = 5000; pricex100 < 10000; pricex100 += 500) {
		ASSERT_EQ(range.index_to_pricex100(i), pricex100);
		ASSERT_EQ(range[i], pricex100);

		i++;
	}

	for (uint64_t pricex100 = 10000; pricex100 <= 100000; pricex100 += 1000) {
		ASSERT_EQ(range.index_to_pricex100(i), pricex100);
		ASSERT_EQ(range[i], pricex100);

		i++;
	}
}

// Test that the .index_to_price() method returns floating point prices
// correctly for each index.
TEST(price_range_test, index_to_price)
{
	janus::betfair::price_range range;

	// Test each of the tick ranges one-by-one, converting pricex100 to
	// double and asserting that the returned value is as expected.

	uint64_t i = 0;

	for (uint64_t pricex100 = 101; pricex100 < 200; pricex100++) {
		ASSERT_DOUBLE_EQ(range.index_to_price(i), static_cast<double>(pricex100) / 100.);

		i++;
	}

	for (uint64_t pricex100 = 200; pricex100 < 300; pricex100 += 2) {
		ASSERT_DOUBLE_EQ(range.index_to_price(i), static_cast<double>(pricex100) / 100.);

		i++;
	}

	for (uint64_t pricex100 = 300; pricex100 < 400; pricex100 += 5) {
		ASSERT_DOUBLE_EQ(range.index_to_price(i), static_cast<double>(pricex100) / 100.);

		i++;
	}

	for (uint64_t pricex100 = 400; pricex100 < 600; pricex100 += 10) {
		ASSERT_DOUBLE_EQ(range.index_to_price(i), static_cast<double>(pricex100) / 100.);

		i++;
	}

	for (uint64_t pricex100 = 600; pricex100 < 1000; pricex100 += 20) {
		ASSERT_DOUBLE_EQ(range.index_to_price(i), static_cast<double>(pricex100) / 100.);

		i++;
	}

	for (uint64_t pricex100 = 1000; pricex100 < 2000; pricex100 += 50) {
		ASSERT_DOUBLE_EQ(range.index_to_price(i), static_cast<double>(pricex100) / 100.);

		i++;
	}

	for (uint64_t pricex100 = 2000; pricex100 < 3000; pricex100 += 100) {
		ASSERT_DOUBLE_EQ(range.index_to_price(i), static_cast<double>(pricex100) / 100.);

		i++;
	}

	for (uint64_t pricex100 = 3000; pricex100 < 5000; pricex100 += 200) {
		ASSERT_DOUBLE_EQ(range.index_to_price(i), static_cast<double>(pricex100) / 100.);

		i++;
	}

	for (uint64_t pricex100 = 5000; pricex100 < 10000; pricex100 += 500) {
		ASSERT_DOUBLE_EQ(range.index_to_price(i), static_cast<double>(pricex100) / 100.);

		i++;
	}

	for (uint64_t pricex100 = 10000; pricex100 <= 100000; pricex100 += 1000) {
		ASSERT_DOUBLE_EQ(range.index_to_price(i), static_cast<double>(pricex100) / 100.);

		i++;
	}
}

// Test that .pricex100_to_nearest_index() returns the index ofthe nearest
// pricex100 less than or equal to the one specified.
TEST(price_range_test, pricex100_to_nearest_index)
{
	janus::betfair::price_range range;

	// Test each of the tick ranges one-by-one and ensure that we obtain the
	// index of the nearest pricex100, rounding down.

	// Expect all pricex100's below 101 are invalid.
	for (uint64_t pricex100 = 0; pricex100 < 101; pricex100++) {
		ASSERT_EQ(range.pricex100_to_nearest_index(pricex100),
			  janus::betfair::INVALID_PRICE_INDEX);
	}

	uint64_t i = 0;

	// 101 -> 200 have a tick size of 1, so no fuzzy values to check.
	for (uint64_t pricex100 = 101; pricex100 < 200; pricex100++) {
		ASSERT_EQ(range.pricex100_to_nearest_index(pricex100), i);

		i++;
	}

	for (uint64_t pricex100 = 200; pricex100 < 300; pricex100++) {
		ASSERT_EQ(range.pricex100_to_nearest_index(pricex100), i);

		if ((pricex100 + 1) % 2 == 0)
			i++;
	}

	for (uint64_t pricex100 = 300; pricex100 < 400; pricex100++) {
		ASSERT_EQ(range.pricex100_to_nearest_index(pricex100), i);

		if ((pricex100 + 1) % 5 == 0)
			i++;
	}

	for (uint64_t pricex100 = 400; pricex100 < 600; pricex100++) {
		ASSERT_EQ(range.pricex100_to_nearest_index(pricex100), i);

		if ((pricex100 + 1) % 10 == 0)
			i++;
	}

	for (uint64_t pricex100 = 600; pricex100 < 1000; pricex100++) {
		ASSERT_EQ(range.pricex100_to_nearest_index(pricex100), i);

		if ((pricex100 + 1) % 20 == 0)
			i++;
	}

	for (uint64_t pricex100 = 1000; pricex100 < 2000; pricex100++) {
		ASSERT_EQ(range.pricex100_to_nearest_index(pricex100), i);

		if ((pricex100 + 1) % 50 == 0)
			i++;
	}

	for (uint64_t pricex100 = 2000; pricex100 < 3000; pricex100++) {
		ASSERT_EQ(range.pricex100_to_nearest_index(pricex100), i);

		if ((pricex100 + 1) % 100 == 0)
			i++;
	}

	for (uint64_t pricex100 = 3000; pricex100 < 5000; pricex100++) {
		ASSERT_EQ(range.pricex100_to_nearest_index(pricex100), i);

		if ((pricex100 + 1) % 200 == 0)
			i++;
	}

	for (uint64_t pricex100 = 5000; pricex100 < 10000; pricex100++) {
		ASSERT_EQ(range.pricex100_to_nearest_index(pricex100), i);

		if ((pricex100 + 1) % 500 == 0)
			i++;
	}

	for (uint64_t pricex100 = 10000; pricex100 <= 100000; pricex100++) {
		ASSERT_EQ(range.pricex100_to_nearest_index(pricex100), i);

		if ((pricex100 + 1) % 1000 == 0)
			i++;
	}
}

// Test that .pricex100_to_index() returns the correct index or
// INVALID_PRICE_INDEX otherwise.
TEST(price_range_test, pricex100_to_index)
{
	janus::betfair::price_range range;

	// Test each of the tick ranges one-by-one and ensure that we obtain the
	// index or INVALID_PRICE_INDEX if the pricex100 we have specified is
	// not in the correct range.

	// Expect all pricex100's below 101 are invalid.
	for (uint64_t pricex100 = 0; pricex100 < 101; pricex100++) {
		ASSERT_EQ(range.pricex100_to_index(pricex100), janus::betfair::INVALID_PRICE_INDEX);
	}

	uint64_t i = 0;

	// 101 -> 200 have a tick size of 1, so no fuzzy values to check.
	for (uint64_t pricex100 = 101; pricex100 < 200; pricex100++) {
		ASSERT_EQ(range.pricex100_to_index(pricex100), i);

		i++;
	}

	for (uint64_t pricex100 = 200; pricex100 < 300; pricex100++) {
		if (pricex100 % 2 == 0) {
			ASSERT_EQ(range.pricex100_to_index(pricex100), i);
			i++;
		} else {
			ASSERT_EQ(range.pricex100_to_index(pricex100),
				  janus::betfair::INVALID_PRICE_INDEX);
		}
	}

	for (uint64_t pricex100 = 300; pricex100 < 400; pricex100++) {
		if (pricex100 % 5 == 0) {
			ASSERT_EQ(range.pricex100_to_index(pricex100), i);
			i++;
		} else {
			ASSERT_EQ(range.pricex100_to_index(pricex100),
				  janus::betfair::INVALID_PRICE_INDEX);
		}
	}

	for (uint64_t pricex100 = 400; pricex100 < 600; pricex100++) {
		if (pricex100 % 10 == 0) {
			ASSERT_EQ(range.pricex100_to_index(pricex100), i);
			i++;
		} else {
			ASSERT_EQ(range.pricex100_to_index(pricex100),
				  janus::betfair::INVALID_PRICE_INDEX);
		}
	}

	for (uint64_t pricex100 = 600; pricex100 < 1000; pricex100++) {
		if (pricex100 % 20 == 0) {
			ASSERT_EQ(range.pricex100_to_index(pricex100), i);
			i++;
		} else {
			ASSERT_EQ(range.pricex100_to_index(pricex100),
				  janus::betfair::INVALID_PRICE_INDEX);
		}
	}

	for (uint64_t pricex100 = 1000; pricex100 < 2000; pricex100++) {
		if (pricex100 % 50 == 0) {
			ASSERT_EQ(range.pricex100_to_index(pricex100), i);
			i++;
		} else {
			ASSERT_EQ(range.pricex100_to_index(pricex100),
				  janus::betfair::INVALID_PRICE_INDEX);
		}
	}

	for (uint64_t pricex100 = 2000; pricex100 < 3000; pricex100++) {
		if (pricex100 % 100 == 0) {
			ASSERT_EQ(range.pricex100_to_index(pricex100), i);
			i++;
		} else {
			ASSERT_EQ(range.pricex100_to_index(pricex100),
				  janus::betfair::INVALID_PRICE_INDEX);
		}
	}

	for (uint64_t pricex100 = 3000; pricex100 < 5000; pricex100++) {
		if (pricex100 % 200 == 0) {
			ASSERT_EQ(range.pricex100_to_index(pricex100), i);
			i++;
		} else {
			ASSERT_EQ(range.pricex100_to_index(pricex100),
				  janus::betfair::INVALID_PRICE_INDEX);
		}
	}

	for (uint64_t pricex100 = 5000; pricex100 < 10000; pricex100++) {
		if (pricex100 % 500 == 0) {
			ASSERT_EQ(range.pricex100_to_index(pricex100), i);
			i++;
		} else {
			ASSERT_EQ(range.pricex100_to_index(pricex100),
				  janus::betfair::INVALID_PRICE_INDEX);
		}
	}

	for (uint64_t pricex100 = 10000; pricex100 <= 100000; pricex100++) {
		if (pricex100 % 1000 == 0) {
			ASSERT_EQ(range.pricex100_to_index(pricex100), i);
			i++;
		} else {
			ASSERT_EQ(range.pricex100_to_index(pricex100),
				  janus::betfair::INVALID_PRICE_INDEX);
		}
	}
}

// Test that .nearest_pricex100() returns the nearest pricex100 (rounding down)
// to the specified pricex100 or INVALID_PRICEX100 if unable to determine
// (e.g. pricex100 < 101).
TEST(price_range_test, nearest_pricex100)
{
	janus::betfair::price_range range;

	// Test each of the tick ranges one-by-one and ensure that we obtain the
	// nearest pricex100, or INVALID_PRICEX100 if unable to determine.

	// Expect all pricex100's below 101 are invalid.
	for (uint64_t pricex100 = 0; pricex100 < 101; pricex100++) {
		ASSERT_EQ(range.nearest_pricex100(pricex100), janus::betfair::INVALID_PRICEX100);
	}

	uint64_t i = 0;

	// 101 -> 200 have a tick size of 1, so no fuzzy values to check.
	for (uint64_t pricex100 = 101; pricex100 < 200; pricex100++) {
		ASSERT_EQ(range.nearest_pricex100(pricex100), pricex100);

		i++;
	}

	for (uint64_t pricex100 = 200; pricex100 < 300; pricex100++) {
		ASSERT_EQ(range.nearest_pricex100(pricex100), range[i]);

		if ((pricex100 + 1) % 2 == 0)
			i++;
	}

	for (uint64_t pricex100 = 300; pricex100 < 400; pricex100++) {
		ASSERT_EQ(range.nearest_pricex100(pricex100), range[i]);

		if ((pricex100 + 1) % 5 == 0)
			i++;
	}

	for (uint64_t pricex100 = 400; pricex100 < 600; pricex100++) {
		ASSERT_EQ(range.nearest_pricex100(pricex100), range[i]);

		if ((pricex100 + 1) % 10 == 0)
			i++;
	}

	for (uint64_t pricex100 = 600; pricex100 < 1000; pricex100++) {
		ASSERT_EQ(range.nearest_pricex100(pricex100), range[i]);

		if ((pricex100 + 1) % 20 == 0)
			i++;
	}

	for (uint64_t pricex100 = 1000; pricex100 < 2000; pricex100++) {
		ASSERT_EQ(range.nearest_pricex100(pricex100), range[i]);

		if ((pricex100 + 1) % 50 == 0)
			i++;
	}

	for (uint64_t pricex100 = 2000; pricex100 < 3000; pricex100++) {
		ASSERT_EQ(range.nearest_pricex100(pricex100), range[i]);

		if ((pricex100 + 1) % 100 == 0)
			i++;
	}

	for (uint64_t pricex100 = 3000; pricex100 < 5000; pricex100++) {
		ASSERT_EQ(range.nearest_pricex100(pricex100), range[i]);

		if ((pricex100 + 1) % 200 == 0)
			i++;
	}

	for (uint64_t pricex100 = 5000; pricex100 < 10000; pricex100++) {
		ASSERT_EQ(range.nearest_pricex100(pricex100), range[i]);

		if ((pricex100 + 1) % 500 == 0)
			i++;
	}

	for (uint64_t pricex100 = 10000; pricex100 <= 100000; pricex100++) {
		ASSERT_EQ(range.nearest_pricex100(pricex100), range[i]);

		if ((pricex100 + 1) % 1000 == 0)
			i++;
	}
}

// Test that the .price_to_nearest_index() method correctly rounds the floating
// point price and returns the index of the nearest pricex100 less than or equal
// to the one specified.
TEST(price_range_test, price_to_nearest_index)
{
	janus::betfair::price_range range;

	// Test each of the tick ranges one-by-one and ensure that we obtain the
	// index of the nearest pricex100, rounding down.

	// Expect all pricex100's below 101 are invalid.
	for (uint64_t pricex100 = 0; pricex100 < 101; pricex100++) {
		double price = static_cast<double>(pricex100 / 100.);
		ASSERT_EQ(range.price_to_nearest_index(price), janus::betfair::INVALID_PRICE_INDEX);
	}

	uint64_t i = 0;

	// 101 -> 200 have a tick size of 1, so no fuzzy values to check.
	for (uint64_t pricex100 = 101; pricex100 < 200; pricex100++) {
		double price = static_cast<double>(pricex100 / 100.);
		ASSERT_EQ(range.price_to_nearest_index(price), i);
		ASSERT_EQ(range.price_to_nearest_index(price - 9.875e-7), i);
		ASSERT_EQ(range.price_to_nearest_index(price + 9.875e-7), i);

		i++;
	}

	for (uint64_t pricex100 = 200; pricex100 < 300; pricex100++) {
		double price = static_cast<double>(pricex100 / 100.);
		ASSERT_EQ(range.price_to_nearest_index(price), i);
		ASSERT_EQ(range.price_to_nearest_index(price - 9.875e-7), i);
		ASSERT_EQ(range.price_to_nearest_index(price + 9.875e-7), i);

		if ((pricex100 + 1) % 2 == 0)
			i++;
	}

	for (uint64_t pricex100 = 300; pricex100 < 400; pricex100++) {
		double price = static_cast<double>(pricex100 / 100.);
		ASSERT_EQ(range.price_to_nearest_index(price), i);
		ASSERT_EQ(range.price_to_nearest_index(price - 9.875e-7), i);
		ASSERT_EQ(range.price_to_nearest_index(price + 9.875e-7), i);

		if ((pricex100 + 1) % 5 == 0)
			i++;
	}

	for (uint64_t pricex100 = 400; pricex100 < 600; pricex100++) {
		double price = static_cast<double>(pricex100 / 100.);
		ASSERT_EQ(range.price_to_nearest_index(price), i);
		ASSERT_EQ(range.price_to_nearest_index(price - 9.875e-7), i);
		ASSERT_EQ(range.price_to_nearest_index(price + 9.875e-7), i);

		if ((pricex100 + 1) % 10 == 0)
			i++;
	}

	for (uint64_t pricex100 = 600; pricex100 < 1000; pricex100++) {
		double price = static_cast<double>(pricex100 / 100.);
		ASSERT_EQ(range.price_to_nearest_index(price), i);
		ASSERT_EQ(range.price_to_nearest_index(price - 9.875e-7), i);
		ASSERT_EQ(range.price_to_nearest_index(price + 9.875e-7), i);

		if ((pricex100 + 1) % 20 == 0)
			i++;
	}

	for (uint64_t pricex100 = 1000; pricex100 < 2000; pricex100++) {
		double price = static_cast<double>(pricex100 / 100.);
		ASSERT_EQ(range.price_to_nearest_index(price), i);
		ASSERT_EQ(range.price_to_nearest_index(price - 9.875e-7), i);
		ASSERT_EQ(range.price_to_nearest_index(price + 9.875e-7), i);

		if ((pricex100 + 1) % 50 == 0)
			i++;
	}

	for (uint64_t pricex100 = 2000; pricex100 < 3000; pricex100++) {
		double price = static_cast<double>(pricex100 / 100.);
		ASSERT_EQ(range.price_to_nearest_index(price), i);
		ASSERT_EQ(range.price_to_nearest_index(price - 9.875e-7), i);
		ASSERT_EQ(range.price_to_nearest_index(price + 9.875e-7), i);

		if ((pricex100 + 1) % 100 == 0)
			i++;
	}

	for (uint64_t pricex100 = 3000; pricex100 < 5000; pricex100++) {
		double price = static_cast<double>(pricex100 / 100.);
		ASSERT_EQ(range.price_to_nearest_index(price), i);
		ASSERT_EQ(range.price_to_nearest_index(price - 9.875e-7), i);
		ASSERT_EQ(range.price_to_nearest_index(price + 9.875e-7), i);

		if ((pricex100 + 1) % 200 == 0)
			i++;
	}

	for (uint64_t pricex100 = 5000; pricex100 < 10000; pricex100++) {
		double price = static_cast<double>(pricex100 / 100.);
		ASSERT_EQ(range.price_to_nearest_index(price), i);
		ASSERT_EQ(range.price_to_nearest_index(price - 9.875e-7), i);
		ASSERT_EQ(range.price_to_nearest_index(price + 9.875e-7), i);

		if ((pricex100 + 1) % 500 == 0)
			i++;
	}

	for (uint64_t pricex100 = 10000; pricex100 <= 100000; pricex100++) {
		double price = static_cast<double>(pricex100 / 100.);
		ASSERT_EQ(range.price_to_nearest_index(price), i);
		ASSERT_EQ(range.price_to_nearest_index(price - 9.875e-7), i);
		ASSERT_EQ(range.price_to_nearest_index(price + 9.875e-7), i);

		if ((pricex100 + 1) % 1000 == 0)
			i++;
	}
}

// Test that the .price_to_nearest_index() method rounds specifically offset
// floating point numbers correctly.
TEST(price_range_test, price_to_nearest_index_rounding)
{
	janus::betfair::price_range range;

	EXPECT_EQ(range.price_to_nearest_index(1.009999547), 0);
	EXPECT_EQ(range.price_to_nearest_index(1.014566667), 0);
	EXPECT_EQ(range.price_to_nearest_index(6.1999923823), range.pricex100_to_index(620));
}

// Test that the .price_to_nearest_pricex100() method correctly rounds the
// floating point price and returns the nearest pricex100 less than or equal to
// the one specified. Leave out rounding errors as covered by index test.
TEST(price_range_test, price_to_nearest_pricex100)
{
	janus::betfair::price_range range;

	// Test each of the tick ranges one-by-one and ensure that we obtain the
	// index of the nearest pricex100, rounding down.

	// Expect all pricex100's below 101 are invalid.
	for (uint64_t pricex100 = 0; pricex100 < 101; pricex100++) {
		double price = static_cast<double>(pricex100 / 100.);
		ASSERT_EQ(range.price_to_nearest_pricex100(price),
			  janus::betfair::INVALID_PRICEX100);
	}

	uint64_t i = 0;

	// 101 -> 200 have a tick size of 1, so no fuzzy values to check.
	for (uint64_t pricex100 = 101; pricex100 < 200; pricex100++) {
		double price = static_cast<double>(pricex100 / 100.);
		ASSERT_EQ(range.price_to_nearest_pricex100(price), pricex100);

		i++;
	}

	for (uint64_t pricex100 = 200; pricex100 < 300; pricex100++) {
		double price = static_cast<double>(pricex100 / 100.);
		ASSERT_EQ(range.price_to_nearest_pricex100(price), range[i]);

		if ((pricex100 + 1) % 2 == 0)
			i++;
	}

	for (uint64_t pricex100 = 300; pricex100 < 400; pricex100++) {
		double price = static_cast<double>(pricex100 / 100.);
		ASSERT_EQ(range.price_to_nearest_pricex100(price), range[i]);

		if ((pricex100 + 1) % 5 == 0)
			i++;
	}

	for (uint64_t pricex100 = 400; pricex100 < 600; pricex100++) {
		double price = static_cast<double>(pricex100 / 100.);
		ASSERT_EQ(range.price_to_nearest_pricex100(price), range[i]);

		if ((pricex100 + 1) % 10 == 0)
			i++;
	}

	for (uint64_t pricex100 = 600; pricex100 < 1000; pricex100++) {
		double price = static_cast<double>(pricex100 / 100.);
		ASSERT_EQ(range.price_to_nearest_pricex100(price), range[i]);

		if ((pricex100 + 1) % 20 == 0)
			i++;
	}

	for (uint64_t pricex100 = 1000; pricex100 < 2000; pricex100++) {
		double price = static_cast<double>(pricex100 / 100.);
		ASSERT_EQ(range.price_to_nearest_pricex100(price), range[i]);

		if ((pricex100 + 1) % 50 == 0)
			i++;
	}

	for (uint64_t pricex100 = 2000; pricex100 < 3000; pricex100++) {
		double price = static_cast<double>(pricex100 / 100.);
		ASSERT_EQ(range.price_to_nearest_pricex100(price), range[i]);

		if ((pricex100 + 1) % 100 == 0)
			i++;
	}

	for (uint64_t pricex100 = 3000; pricex100 < 5000; pricex100++) {
		double price = static_cast<double>(pricex100 / 100.);
		ASSERT_EQ(range.price_to_nearest_pricex100(price), range[i]);

		if ((pricex100 + 1) % 200 == 0)
			i++;
	}

	for (uint64_t pricex100 = 5000; pricex100 < 10000; pricex100++) {
		double price = static_cast<double>(pricex100 / 100.);
		ASSERT_EQ(range.price_to_nearest_pricex100(price), range[i]);

		if ((pricex100 + 1) % 500 == 0)
			i++;
	}

	for (uint64_t pricex100 = 10000; pricex100 <= 100000; pricex100++) {
		double price = static_cast<double>(pricex100 / 100.);
		ASSERT_EQ(range.price_to_nearest_pricex100(price), range[i]);

		if ((pricex100 + 1) % 1000 == 0)
			i++;
	}
}

// Test that .pricex100_to_nearest_index_up() returns the index ofthe nearest
// pricex100 greater than or equal to the one specified.
TEST(price_range_test, pricex100_to_nearest_index_up)
{
	janus::betfair::price_range range;

	// Test each of the tick ranges one-by-one and ensure that we obtain the
	// index of the nearest pricex100, rounding up.

	// Expect all pricex100's below 101 are invalid.
	for (uint64_t pricex100 = 0; pricex100 < 101; pricex100++) {
		ASSERT_EQ(range.pricex100_to_nearest_index_up(pricex100),
			  janus::betfair::INVALID_PRICE_INDEX);
	}

	uint64_t i = 0;

	// 101 -> 200 have a tick size of 1, so no fuzzy values to check.
	for (uint64_t pricex100 = 101; pricex100 < 200; pricex100++) {
		ASSERT_EQ(range.pricex100_to_nearest_index_up(pricex100), i);

		i++;
	}

	for (uint64_t pricex100 = 200; pricex100 < 300; pricex100++) {
		ASSERT_EQ(range.pricex100_to_nearest_index_up(pricex100), i);

		if (pricex100 % 2 == 0)
			i++;
	}

	for (uint64_t pricex100 = 300; pricex100 < 400; pricex100++) {
		ASSERT_EQ(range.pricex100_to_nearest_index_up(pricex100), i);

		if (pricex100 % 5 == 0)
			i++;
	}

	for (uint64_t pricex100 = 400; pricex100 < 600; pricex100++) {
		ASSERT_EQ(range.pricex100_to_nearest_index_up(pricex100), i);

		if (pricex100 % 10 == 0)
			i++;
	}

	for (uint64_t pricex100 = 600; pricex100 < 1000; pricex100++) {
		ASSERT_EQ(range.pricex100_to_nearest_index_up(pricex100), i);

		if (pricex100 % 20 == 0)
			i++;
	}

	for (uint64_t pricex100 = 1000; pricex100 < 2000; pricex100++) {
		ASSERT_EQ(range.pricex100_to_nearest_index_up(pricex100), i);

		if (pricex100 % 50 == 0)
			i++;
	}

	for (uint64_t pricex100 = 2000; pricex100 < 3000; pricex100++) {
		ASSERT_EQ(range.pricex100_to_nearest_index_up(pricex100), i);

		if (pricex100 % 100 == 0)
			i++;
	}

	for (uint64_t pricex100 = 3000; pricex100 < 5000; pricex100++) {
		ASSERT_EQ(range.pricex100_to_nearest_index_up(pricex100), i);

		if (pricex100 % 200 == 0)
			i++;
	}

	for (uint64_t pricex100 = 5000; pricex100 < 10000; pricex100++) {
		ASSERT_EQ(range.pricex100_to_nearest_index_up(pricex100), i);

		if (pricex100 % 500 == 0)
			i++;
	}

	for (uint64_t pricex100 = 10000; pricex100 <= 100000; pricex100++) {
		ASSERT_EQ(range.pricex100_to_nearest_index_up(pricex100), i);

		if (pricex100 % 1000 == 0)
			i++;
	}
}

// Test that the .price_to_nearest_index_up() method correctly rounds the
// floating point price and returns the index of the nearest pricex100 greater
// than or equal to the one specified.
TEST(price_range_test, price_to_nearest_index_up)
{
	janus::betfair::price_range range;

	// Test each of the tick ranges one-by-one and ensure that we obtain the
	// index of the nearest pricex100, rounding down.

	// Expect all pricex100's below 101 are invalid.
	for (uint64_t pricex100 = 0; pricex100 < 101; pricex100++) {
		double price = static_cast<double>(pricex100 / 100.);
		ASSERT_EQ(range.price_to_nearest_index_up(price),
			  janus::betfair::INVALID_PRICE_INDEX);
	}

	uint64_t i = 0;

	// 101 -> 200 have a tick size of 1, so no fuzzy values to check.
	for (uint64_t pricex100 = 101; pricex100 < 200; pricex100++) {
		double price = static_cast<double>(pricex100 / 100.);
		ASSERT_EQ(range.price_to_nearest_index_up(price), i) << price;
		ASSERT_EQ(range.price_to_nearest_index_up(price - 9.875e-7), i);
		ASSERT_EQ(range.price_to_nearest_index_up(price + 9.875e-7), i);

		i++;
	}

	for (uint64_t pricex100 = 200; pricex100 < 300; pricex100++) {
		double price = static_cast<double>(pricex100 / 100.);
		ASSERT_EQ(range.price_to_nearest_index_up(price), i);
		ASSERT_EQ(range.price_to_nearest_index_up(price - 9.875e-7), i);
		ASSERT_EQ(range.price_to_nearest_index_up(price + 9.875e-7), i);

		if (pricex100 % 2 == 0)
			i++;
	}

	for (uint64_t pricex100 = 300; pricex100 < 400; pricex100++) {
		double price = static_cast<double>(pricex100 / 100.);
		ASSERT_EQ(range.price_to_nearest_index_up(price), i);
		ASSERT_EQ(range.price_to_nearest_index_up(price - 9.875e-7), i);
		ASSERT_EQ(range.price_to_nearest_index_up(price + 9.875e-7), i);

		if (pricex100 % 5 == 0)
			i++;
	}

	for (uint64_t pricex100 = 400; pricex100 < 600; pricex100++) {
		double price = static_cast<double>(pricex100 / 100.);
		ASSERT_EQ(range.price_to_nearest_index_up(price), i);
		ASSERT_EQ(range.price_to_nearest_index_up(price - 9.875e-7), i);
		ASSERT_EQ(range.price_to_nearest_index_up(price + 9.875e-7), i);

		if (pricex100 % 10 == 0)
			i++;
	}

	for (uint64_t pricex100 = 600; pricex100 < 1000; pricex100++) {
		double price = static_cast<double>(pricex100 / 100.);
		ASSERT_EQ(range.price_to_nearest_index_up(price), i);
		ASSERT_EQ(range.price_to_nearest_index_up(price - 9.875e-7), i);
		ASSERT_EQ(range.price_to_nearest_index_up(price + 9.875e-7), i);

		if (pricex100 % 20 == 0)
			i++;
	}

	for (uint64_t pricex100 = 1000; pricex100 < 2000; pricex100++) {
		double price = static_cast<double>(pricex100 / 100.);
		ASSERT_EQ(range.price_to_nearest_index_up(price), i);
		ASSERT_EQ(range.price_to_nearest_index_up(price - 9.875e-7), i);
		ASSERT_EQ(range.price_to_nearest_index_up(price + 9.875e-7), i);

		if (pricex100 % 50 == 0)
			i++;
	}

	for (uint64_t pricex100 = 2000; pricex100 < 3000; pricex100++) {
		double price = static_cast<double>(pricex100 / 100.);
		ASSERT_EQ(range.price_to_nearest_index_up(price), i);
		ASSERT_EQ(range.price_to_nearest_index_up(price - 9.875e-7), i);
		ASSERT_EQ(range.price_to_nearest_index_up(price + 9.875e-7), i);

		if (pricex100 % 100 == 0)
			i++;
	}

	for (uint64_t pricex100 = 3000; pricex100 < 5000; pricex100++) {
		double price = static_cast<double>(pricex100 / 100.);
		ASSERT_EQ(range.price_to_nearest_index_up(price), i);
		ASSERT_EQ(range.price_to_nearest_index_up(price - 9.875e-7), i);
		ASSERT_EQ(range.price_to_nearest_index_up(price + 9.875e-7), i);

		if (pricex100 % 200 == 0)
			i++;
	}

	for (uint64_t pricex100 = 5000; pricex100 < 10000; pricex100++) {
		double price = static_cast<double>(pricex100 / 100.);
		ASSERT_EQ(range.price_to_nearest_index_up(price), i);
		ASSERT_EQ(range.price_to_nearest_index_up(price - 9.875e-7), i);
		ASSERT_EQ(range.price_to_nearest_index_up(price + 9.875e-7), i);

		if (pricex100 % 500 == 0)
			i++;
	}

	for (uint64_t pricex100 = 10000; pricex100 <= 100000; pricex100++) {
		double price = static_cast<double>(pricex100 / 100.);
		ASSERT_EQ(range.price_to_nearest_index_up(price), i);
		ASSERT_EQ(range.price_to_nearest_index_up(price - 9.875e-7), i);
		ASSERT_EQ(range.price_to_nearest_index_up(price + 9.875e-7), i);

		if (pricex100 % 1000 == 0)
			i++;
	}
}
} // namespace
