#pragma once

#include <array>
#include <cstdint>
#include <string_view>

namespace janus
{
namespace internal
{
static constexpr uint64_t MIN_YEAR = 1970;
static constexpr uint64_t MAX_YEAR = 2100;
static constexpr uint64_t NUM_YEARS = MAX_YEAR - MIN_YEAR + 1;

// Parse digits for a fixed size integer = log10(start_mult), e.g. parse_digits<100>("123");
//   start_mult: The multiplier of the left-most digit, e.g. 10^(num_digits - 1)
//          str: The string to be parsed.
template<uint64_t start_mult>
static inline auto parse_digits(const char* str) -> uint64_t
{
	uint64_t ret = 0;

	for (uint64_t mult = start_mult; mult > 0; mult /= 10) { // NOLINT: Not magical.
		ret += mult * static_cast<uint64_t>(*str++ - '0');
	}

	return ret;
}

// Print digits to the specified char buffer and update the buffer pointer to
// point at the end of it. If log10(start_mult) > num_digits, leading zeroes
// will be printed.
//   start_mult: The multiplier of the left-most digit, e.g. 10^(num_digits - 1)
//          str: Pointer to char buffer to output to.
//      returns: Pointer to character after written digits.
template<uint64_t start_mult>
static inline auto print_digits(char* str, uint64_t num) -> char*
{
	for (uint64_t mult = start_mult; mult > 0; mult /= 10) { // NOLINT: Not magical.
		*str++ = '0' + (num / mult) % 10;                // NOLINT: Not magical.
	}

	return str;
}

// Determine if the specified year is a leap year.
static inline auto is_leap(uint64_t year) -> bool
{
	/*
	 * Every year that is exactly divisible by four is a leap year, except for years
	 * that are exactly divisible by 100, but these centurial years are leap years
	 * if they are exactly divisible by 400. For example, the years 1700, 1800, and
	 * 1900 are not leap years, but the years 1600 and 2000 are.
	 *
	 * https://en.wikipedia.org/wiki/Leap_year
	 */
	if ((year % 4) != 0) // NOLINT: Not magical.
		return false;

	return (year % 400) == 0 || (year % 100) != 0; // NOLINT: Not magical.
}
} // namespace internal

auto parse_iso8601(const char* str, uint64_t size) -> uint64_t;
auto print_iso8601(char* str, uint64_t epoch_ms) -> std::string_view;
} // namespace janus
