#pragma once

#include <cmath>
#include <fstream>
#include <string>

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

// Does the file at the specified path exist?
static inline auto file_exists(const std::string& path) -> bool
{
	std::ifstream f(path);
	return f.good();
}
} // namespace janus
