#include "decimal7.hh"

#include <gtest/gtest.h>

namespace {
TEST(Decimal7Test, Ctor)
{
	janus::decimal7 d;

	EXPECT_EQ(d.raw_encoded(), 0);
}
} // empty namespace
