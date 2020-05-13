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
} // namespace janus
