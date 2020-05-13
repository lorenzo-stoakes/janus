#pragma once

#include <cmath>

namespace janus
{
static constexpr double DOUBLE_EPSILON = 1e-15;

// Compare double values with small error bar.
static inline auto deq(double a, double b) -> bool
{
	double diff = ::fabs(a - b);
	return diff < DOUBLE_EPSILON;
}

// Compare double value to zero with small error bar.
static inline auto dz(double val) -> bool
{
	return deq(val, 0);
}
} // namespace janus
