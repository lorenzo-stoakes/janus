#pragma once

#include <cstdint>
#include <cstdlib>
#include <string_view>

namespace janus
{
static constexpr uint64_t MS_PER_HOUR = 1000 * 60 * 60;
static constexpr uint64_t MS_PER_DAY = MS_PER_HOUR * 24;

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

// Parse a market ID in format x.yyyy... (where x is typically 1).
//     str: Null-terminated string of size at least 3.
//    size: Size of input string.
// returns: uint64 of digits after prefix (e.g. yyyy).
static inline auto parse_market_id(const char* str, uint64_t size) -> uint64_t
{
	if (size < 3)
		return 0;

	return static_cast<uint64_t>(std::atol(&str[2]));
}
} // namespace internal

// Convert an epoch_ms value to separate date/time component values.
void unpack_epoch_ms(uint64_t epoch_ms, uint64_t& year, uint64_t& month, uint64_t& day,
		     uint64_t& hour, uint64_t& minute, uint64_t& second, uint64_t& ms);

// Convert an epoch_ms value to separate date/time component values in the local
// timezone.
void local_unpack_epoch_ms(uint64_t epoch_ms, uint64_t& year, uint64_t& month, uint64_t& day,
			   uint64_t& hour, uint64_t& minute, uint64_t& second, uint64_t& ms);

// Convert local epoch ms to yyyy-MM-dd HH:mm:ss format.
auto local_epoch_ms_to_string(uint64_t epoch_ms) -> std::string;

// Encode date components into days since epoch.
auto encode_epoch_days(uint64_t year, uint64_t month, uint64_t day) -> uint64_t;

// Encode date/time components into ms since epoch.
auto encode_epoch(uint64_t year, uint64_t month, uint64_t day, uint64_t hour, uint64_t minute,
		  uint64_t second, uint64_t ms) -> uint64_t;

// Parse an ISO-8601 format (e.g. 2020-03-11T13:20:00.123Z) and convert it to time
// since epoch in ms.
//     str: The string to be parsed. Must be in zulu timezone (GMT).
//    size: Size of the string to be parsed.
// returns: The number of ms since epoch (1/1/1970 00:00), or 0 if cannot be
//          parsed.
auto parse_iso8601(const char* str, uint64_t size) -> uint64_t;

// Convert ms since epoch value to an ISO-8601 string. It outputs to a 25 byte
// character buffer including ms, e.g. 2020-03-11T13:20:00.123Z.
//   epoch_ms: Milliseconds since epoch (1/1/1970 00:00)
//        str: Character buffer containing minimum 25 bytes space where string will be output to
//    returns: Returns a std::string_view pointing at the provided char buffer for convenience.
auto print_iso8601(char* str, uint64_t epoch_ms) -> std::string_view;
} // namespace janus
