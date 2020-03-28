#include "decimal7.hh"

#include <gtest/gtest.h>

namespace
{
// Test that constructors behave as expected.
TEST(Decimal7Test, Ctor)
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
}

// Test that the equality operators behave as expected.
TEST(Decimal7Test, Equality)
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
} // namespace
