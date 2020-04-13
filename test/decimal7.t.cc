#include "janus.hh"

#include <cstdint>
#include <gtest/gtest.h>
#include <stdexcept>

namespace
{
// Test that constructors behave as expected.
TEST(decimal7_test, ctor)
{
	// Default.
	janus::decimal7 empty;
	EXPECT_EQ(empty.raw_encoded(), 0);

	// Simple case.
	janus::decimal7 d1(1234, 3);
	EXPECT_EQ(d1.raw(), 1234);
	EXPECT_EQ(d1.num_places(), 3);

	// Negative simple case.
	janus::decimal7 d2(-1234, 3);
	EXPECT_EQ(d2.raw(), -1234);
	EXPECT_EQ(d2.num_places(), 3);

	// Leading zeroes.
	janus::decimal7 d3(1, 7);
	EXPECT_EQ(d3.raw(), 1);
	EXPECT_EQ(d3.num_places(), 7);
	EXPECT_EQ(d3.int64(), 0);
	// Note the stripped leading zeroes.
	EXPECT_EQ(d3.fract(), 1);
	EXPECT_DOUBLE_EQ(d3.to_double(), 0.0000001);

	// Leading zeroes with more decimal places.
	janus::decimal7 d4(123, 7);
	EXPECT_EQ(d4.raw(), 123);
	EXPECT_EQ(d4.num_places(), 7);
	EXPECT_EQ(d4.int64(), 0);
	EXPECT_EQ(d4.fract(), 123);
	EXPECT_DOUBLE_EQ(d4.to_double(), 0.0000123);

	// Too many places.
	janus::decimal7 d5(123456789012, 12);
	EXPECT_EQ(d5.raw(), 1234567);
	EXPECT_EQ(d5.num_places(), 7);
	EXPECT_EQ(d5.int64(), 0);
	EXPECT_EQ(d5.fract(), 1234567);
	EXPECT_DOUBLE_EQ(d5.to_double(), 0.1234567);

	// Zero places.
	janus::decimal7 d6(123, 0);
	EXPECT_EQ(d6.raw(), 123);
	EXPECT_EQ(d6.num_places(), 0);
	EXPECT_EQ(d6.int64(), 123);
	EXPECT_EQ(d6.fract(), 0);

	// Trailing zeroes.
	janus::decimal7 d7(123400, 3);
	EXPECT_EQ(d7.raw(), 1234);
	EXPECT_EQ(d7.num_places(), 1);
	EXPECT_EQ(d7.int64(), 123);
	EXPECT_EQ(d7.fract(), 4);

	// Trailing zeroes to integer.
	janus::decimal7 d8(123400, 2);
	EXPECT_EQ(d8.raw(), 1234);
	EXPECT_EQ(d8.num_places(), 0);
	EXPECT_EQ(d8.int64(), 1234);
	EXPECT_EQ(d8.fract(), 0);

	// Zero values with trailing zeroes.
	for (uint8_t i = 0; i < 8; i++) {
		janus::decimal7 d9(0, i);
		EXPECT_EQ(d9.raw(), 0);
		EXPECT_EQ(d9.num_places(), 0);
		EXPECT_EQ(d9.raw_encoded(), 0);
		EXPECT_EQ(d9.int64(), 0);
		EXPECT_EQ(d9.fract(), 0);
	}
}

// Test that the equality operators behave as expected.
TEST(decimal7_test, equality)
{
	janus::decimal7 d1(123456, 3);
	janus::decimal7 d2(123456, 3);
	EXPECT_EQ(d1, d1);
	EXPECT_EQ(d2, d2);
	EXPECT_EQ(d1, d2);

	janus::decimal7 d3(-654321, 2);
	EXPECT_NE(d1, d3);
	EXPECT_NE(d2, d3);
}

// Test that values with trailing zeroes in the fractional portion of the value
// but with equivalent values are in fact considered equal (e.g. 1.230 == 1.23).
TEST(decimal7_test, trailing_zero_equality)
{
	janus::decimal7 d1(1230, 3);
	janus::decimal7 d2(123, 2);
	EXPECT_EQ(d1, d2);

	janus::decimal7 d3(-1230, 3);
	janus::decimal7 d4(-123, 2);
	EXPECT_EQ(d3, d4);

	janus::decimal7 d5(123000, 3);
	janus::decimal7 d6(123, 0);
	EXPECT_EQ(d5, d6);

	// Zero values with trailing zeroes should also be equal.
	janus::decimal7 d7(0, 0);
	for (uint8_t i = 0; i < 8; i++) {
		janus::decimal7 d8(0, i);

		EXPECT_EQ(d7, d8);
	}
}

// Ensure that the .num_places() method returns sane values in all cases.
TEST(decimal7_test, num_places)
{
	// Standard cases, from 0 - 7 decimal places.
	int64_t n = 1;
	for (uint8_t i = 0; i < 8; i++) {
		janus::decimal7 d1(n, i);

		EXPECT_EQ(d1.num_places(), i);
		EXPECT_EQ(d1.int64(), 1);

		janus::decimal7 d2(-n, i);
		EXPECT_EQ(d2.num_places(), i);
		EXPECT_EQ(d2.raw(), -n);
		EXPECT_EQ(d2.int64(), -1);

		n *= 10;
		n += i + 1;
	}

	// Try some very large numbers.
	for (uint8_t i = 0; i < 8; i++) {
		janus::decimal7 d3(janus::decimal7::MAX, i);
		EXPECT_EQ(d3.num_places(), i);
		EXPECT_EQ(d3.raw(), janus::decimal7::MAX);

		janus::decimal7 d4(janus::decimal7::MIN, i);
		EXPECT_EQ(d4.num_places(), i);
		EXPECT_EQ(d4.raw(), janus::decimal7::MIN);
	}

	// To risk duplicating other tests here, trailing zeroes should always
	// return 0.
	for (uint8_t i = 0; i < 8; i++) {
		janus::decimal7 d5(0, i);
		EXPECT_EQ(d5.num_places(), 0);

		janus::decimal7 d6(1'230'000'000, i);
		EXPECT_EQ(d6.num_places(), 0);

		janus::decimal7 d7(-1'230'000'000, i);
		EXPECT_EQ(d7.num_places(), 0);

		if (i >= 3) {
			janus::decimal7 d8(1'234'567'000, i);
			EXPECT_EQ(d8.num_places(), i - 3);
		}
	}
}

// Ensure that the .int64() method returns sane values in all cases.
TEST(decimal7_test, int64)
{
	// Some basic cases.

	janus::decimal7 d1(0, 0);
	EXPECT_EQ(d1.int64(), 0);

	janus::decimal7 d2(1, 0);
	EXPECT_EQ(d2.int64(), 1);

	janus::decimal7 d3(-1, 0);
	EXPECT_EQ(d3.int64(), -1);

	// Large values.

	janus::decimal7 d4(janus::decimal7::MAX, 0);
	EXPECT_EQ(d4.int64(), janus::decimal7::MAX);

	janus::decimal7 d5(janus::decimal7::MIN, 0);
	EXPECT_EQ(d5.int64(), janus::decimal7::MIN);

	// Now large values with decimal places.
	int64_t mult = 1;
	for (uint8_t i = 0; i < 8; i++) {
		janus::decimal7 d6(janus::decimal7::MAX, i);
		EXPECT_EQ(d6.int64(), janus::decimal7::MAX / mult);

		janus::decimal7 d7(janus::decimal7::MIN, i);
		EXPECT_EQ(d7.int64(), janus::decimal7::MIN / mult);

		mult *= 10;
	}
}

// Ensure that the .fract() method returns sane values in all cases.
TEST(decimal7_test, fract)
{
	// Some basic cases.

	janus::decimal7 d1(0, 0);
	EXPECT_EQ(d1.fract(), 0);

	janus::decimal7 d2(123, 2);
	EXPECT_EQ(d2.fract(), 23);

	janus::decimal7 d3(-123, 2);
	EXPECT_EQ(d3.fract(), 23);

	// Now large values with decimal places.
	int64_t mult = 1;
	for (uint8_t i = 0; i < 8; i++) {
		janus::decimal7 d4(janus::decimal7::MAX, i);
		EXPECT_EQ(d4.fract(), janus::decimal7::MAX % mult);

		janus::decimal7 d5(janus::decimal7::MIN, i);
		EXPECT_EQ(d5.fract(), -janus::decimal7::MIN % mult);

		mult *= 10;
	}

	// Leading zeroes. The .fract() method strips these altogether.
	for (uint8_t i = 1; i < 8; i++) {
		janus::decimal7 d6(1'230'000'001LL, i);
		EXPECT_EQ(d6.fract(), 1);
		janus::decimal7 d7(-1'230'000'001LL, i);
		EXPECT_EQ(d7.fract(), 1);

		janus::decimal7 d8(2, i);
		EXPECT_EQ(d8.fract(), 2);
		janus::decimal7 d9(-2, i);
		EXPECT_EQ(d9.fract(), 2);
	}

	// Trailing zeroes.
	for (uint8_t i = 0; i < 8; i++) {
		janus::decimal7 d10(1'230'000'000LL, i);
		EXPECT_EQ(d10.fract(), 0);

		if (i >= 5) {
			janus::decimal7 d11(1'230'000'042'000LL, i);
			EXPECT_EQ(d11.fract(), 42);
		}

		// 0.00...i
		janus::decimal7 d12(i, 7);
		EXPECT_EQ(d12.fract(), i);
	}
	janus::decimal7 d13(12, 1);
	janus::decimal7 d14(120, 2);
	EXPECT_EQ(d13.fract(), 2);
	EXPECT_EQ(d14.fract(), 2);
}

// Ensure that the .to_double() method returns sane values in all cases.
TEST(decimal7_test, to_double)
{
	// Some basic cases.

	janus::decimal7 d1(1234, 3);
	EXPECT_DOUBLE_EQ(d1.to_double(), 1.234);
	janus::decimal7 d2(-1234, 3);
	EXPECT_DOUBLE_EQ(d2.to_double(), -1.234);

	janus::decimal7 d3(1234, 0);
	EXPECT_DOUBLE_EQ(d3.to_double(), 1234);
	janus::decimal7 d4(-1234, 0);
	EXPECT_DOUBLE_EQ(d4.to_double(), -1234);

	janus::decimal7 d5(111, 3);
	EXPECT_DOUBLE_EQ(d5.to_double(), 0.111);
	janus::decimal7 d6(-111, 3);
	EXPECT_DOUBLE_EQ(d6.to_double(), -0.111);

	janus::decimal7 d7(123, 5);
	EXPECT_DOUBLE_EQ(d7.to_double(), 0.00123);
	janus::decimal7 d8(-123, 5);
	EXPECT_DOUBLE_EQ(d8.to_double(), -0.00123);

	// Some larger numbers.

	double max = janus::decimal7::MAX;
	double min = janus::decimal7::MIN;
	for (uint8_t i = 0; i < 8; i++) {
		janus::decimal7 d9(janus::decimal7::MAX, i);
		EXPECT_DOUBLE_EQ(d9.to_double(), max);
		janus::decimal7 d10(janus::decimal7::MIN, i);
		EXPECT_DOUBLE_EQ(d10.to_double(), min);

		max /= 10;
		min /= 10;
	}
}

// Ensure that the .mult10n() method returns sane values in all cases.
TEST(decimal7_test, mult10n)
{
	// Some basic cases.

	janus::decimal7 d1(1234, 3);
	janus::decimal7 d2(-1234, 3);
	int64_t mult = 1000;
	for (uint8_t i = 0; i < 8; i++) {
		if (i < 3) {
			EXPECT_EQ(d1.mult10n(i), 1234 / mult);
			EXPECT_EQ(d2.mult10n(i), -1234 / mult);
			mult /= 10;
		} else {
			EXPECT_EQ(d1.mult10n(i), 1234 * mult);
			EXPECT_EQ(d2.mult10n(i), -1234 * mult);
			mult *= 10;
		}
	}

	// Zeroes should still be zero.
	janus::decimal7 d3(0, 0);
	for (uint8_t i = 0; i < 8; i++) {
		EXPECT_EQ(d3.mult10n(i), 0);
	}

	// All req_places > MAX_PLACES should result in an exception.
	janus::decimal7 d4(123'456'789, 7);
	for (int i = 8; i <= 255; i++) {
		EXPECT_THROW(d4.mult10n(i), std::runtime_error);
	}
}

// Ensure that .mult100(), .mult1000() function as hardcoded cases of mult10n.
TEST(decimal7_test, multx)
{
	// Effectively the same tests as for .mult10n() but using the .mult100()
	// and .mult1000() hardcoded methods.

	janus::decimal7 d1(1234, 3);
	janus::decimal7 d2(-1234, 3);

	EXPECT_EQ(d1.mult100(), 123);
	EXPECT_EQ(d1.mult1000(), 1234);
	EXPECT_EQ(d2.mult100(), -123);
	EXPECT_EQ(d2.mult1000(), -1234);

	janus::decimal7 d3(0, 0);
	EXPECT_EQ(d3.mult100(), 0);
	EXPECT_EQ(d3.mult1000(), 0);
}

// Ensure that .raw() returns the raw integral value of the decimal.
TEST(decimal7_test, raw)
{
	for (uint8_t i = 0; i < 8; i++) {
		// Basic checks.
		janus::decimal7 d1(1234, i);
		EXPECT_EQ(d1.raw(), 1234);
		janus::decimal7 d2(-1234, i);
		EXPECT_EQ(d2.raw(), -1234);

		// Zero should always be zero.
		janus::decimal7 d3(0, i);
		EXPECT_EQ(d3.raw(), 0);

		// Try some large numbers.
		janus::decimal7 d4(janus::decimal7::MAX, i);
		EXPECT_EQ(d4.raw(), janus::decimal7::MAX);
		janus::decimal7 d5(janus::decimal7::MIN, i);
		EXPECT_EQ(d5.raw(), janus::decimal7::MIN);
	}
}

// Ensure that the .raw_encoded() function correclty returns the raw encoded
// value.
TEST(decimal7_test, raw_encoded)
{
	// Without number of places the value will be identical to the raw
	// value.
	janus::decimal7 d1(1234, 0);
	EXPECT_EQ(d1.raw_encoded(), d1.raw());

	janus::decimal7 d2(1234, 0);
	EXPECT_EQ(d2.raw_encoded(), d2.raw());

	// Zero should still be zero.
	janus::decimal7 d3(0, 0);
	EXPECT_EQ(d3.raw_encoded(), 0);

	// Assert that the underlying data size is equal to that of a uint64_t.
	EXPECT_EQ(sizeof(d3), sizeof(uint64_t));

	// Avoid relying on implementation details too much, simply ensure that
	// the underlying value is equal to .raw_encoded().
	for (uint8_t i = 0; i < 8; i++) {
		janus::decimal7 d4(123'456'789'012, i);
		uint64_t underlying = reinterpret_cast<uint64_t&>(d4);
		EXPECT_EQ(underlying, d4.raw_encoded());
	}
}
} // namespace
