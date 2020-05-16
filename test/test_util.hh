#pragma once

#include <cmath>

// Quick and dirty function to round a number to 2 decimal places.
static inline auto round_2dp(double n) -> double
{
	int64_t nx100 = ::llround(n * 100.);
	return static_cast<double>(nx100) / 100;
}
