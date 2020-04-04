#pragma once

#include <array>
#include <cstdint>

namespace janus
{
namespace internal
{
static constexpr uint64_t MIN_YEAR = 1970;
static constexpr uint64_t MAX_YEAR = 2100;
static constexpr uint64_t NUM_YEARS = MAX_YEAR - MIN_YEAR + 1;
// Lookup tables.
static const std::array<uint64_t, 13> DAY_OFFSET_BY_MONTH_NO_LEAP = {
	0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334, 365};
static const std::array<uint64_t, 13> DAY_OFFSET_BY_MONTH_LEAP = {0,   31,  60,  91,  121, 152, 182,
								  213, 244, 274, 305, 335, 366};
static const std::array<uint64_t, NUM_YEARS> DAY_OFFSET_BY_YEAR_SINCE_EPOCH = {
	0,     365,   730,   1096,  1461,  1826,  2191,  2557,  2922,  3287,  3652,  4018,
	4383,  4748,  5113,  5479,  5844,  6209,  6574,  6940,  7305,  7670,  8035,  8401,
	8766,  9131,  9496,  9862,  10227, 10592, 10957, 11323, 11688, 12053, 12418, 12784,
	13149, 13514, 13879, 14245, 14610, 14975, 15340, 15706, 16071, 16436, 16801, 17167,
	17532, 17897, 18262, 18628, 18993, 19358, 19723, 20089, 20454, 20819, 21184, 21550,
	21915, 22280, 22645, 23011, 23376, 23741, 24106, 24472, 24837, 25202, 25567, 25933,
	26298, 26663, 27028, 27394, 27759, 28124, 28489, 28855, 29220, 29585, 29950, 30316,
	30681, 31046, 31411, 31777, 32142, 32507, 32872, 33238, 33603, 33968, 34333, 34699,
	35064, 35429, 35794, 36160, 36525, 36890, 37255, 37621, 37986, 38351, 38716, 39082,
	39447, 39812, 40177, 40543, 40908, 41273, 41638, 42004, 42369, 42734, 43099, 43465,
	43830, 44195, 44560, 44926, 45291, 45656, 46021, 46387, 46752, 47117, 47482,
};

// Convenient units.
static constexpr uint64_t MS_IN_SEC = 1000;
static constexpr uint64_t SECS_IN_MIN = 60;
static constexpr uint64_t MINS_IN_HR = 60;
static constexpr uint64_t HRS_IN_DAY = 24;

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

// Some useful aliases.
static inline auto parse_digits10 = parse_digits<10>;     // NOLINT: Not magical.
static inline auto parse_digits100 = parse_digits<100>;   // NOLINT: Not magical.
static inline auto parse_digits1000 = parse_digits<1000>; // NOLINT: Not magical.

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

static inline auto print_digits10 = print_digits<10>;     // NOLINT: Not magical.
static inline auto print_digits100 = print_digits<100>;   // NOLINT: Not magical.
static inline auto print_digits1000 = print_digits<1000>; // NOLINT: Not magical.

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

// Parse an ISO-8601 format (e.g. 2020-03-11T13:20:00.123Z) and convert it to time
// since epoch in ms.
//     str: The string to be parsed. Must be in zulu timezone (GMT).
//    size: Size of the string to be parsed.
// returns: The number of ms since epoch (1/1/1970 00:00), or 0 if cannot be
//          parsed.
static inline auto parse_iso8601(const char* str, uint64_t size) -> uint64_t // NOLINT: Not used yet
{
	// There are 2 permissible formats:
	//   2020-03-11T13:20:00.123Z (24 chars, with ms)
	//   2020-03-11T13:20:00Z     (20 chars, without)
	static constexpr uint64_t MAX_SIZE = 24;
	static constexpr uint64_t MAX_SIZE_NO_MS = 20;

	// We only support Z = zulu timezone (GMT).
	if ((size != MAX_SIZE && size != MAX_SIZE_NO_MS) || str[size - 1] != 'Z')
		return 0;

	//           1         2
	// 012345678901234567890123
	// ****
	// 2020-03-11T13:20:00.123Z
	static constexpr uint64_t YEAR_INDEX = 0;
	// ****
	// 2020-03-11T13:20:00.123Z
	static constexpr uint64_t MONTH_INDEX = 5;
	//         **
	// 2020-03-11T13:20:00.123Z
	static constexpr uint64_t DAY_INDEX = 8;
	//            **
	// 2020-03-11T13:20:00.123Z
	static constexpr uint64_t HOUR_INDEX = 11;
	//               **
	// 2020-03-11T13:20:00.123Z
	static constexpr uint64_t MINUTE_INDEX = 14;
	//                  **
	// 2020-03-11T13:20:00.123Z
	static constexpr uint64_t SECOND_INDEX = 17;
	//                     ***
	// 2020-03-11T13:20:00.123Z
	static constexpr uint64_t MS_INDEX = 20;

	// For simplicity and speed we assume these values will be valid.
	uint64_t year = internal::parse_digits1000(&str[YEAR_INDEX]);
	uint64_t month = internal::parse_digits10(&str[MONTH_INDEX]);
	uint64_t day = internal::parse_digits10(&str[DAY_INDEX]);
	uint64_t hour = internal::parse_digits10(&str[HOUR_INDEX]);
	uint64_t minute = internal::parse_digits10(&str[MINUTE_INDEX]);
	uint64_t second = internal::parse_digits10(&str[SECOND_INDEX]);
	uint64_t ms = size == MAX_SIZE ? internal::parse_digits100(&str[MS_INDEX]) : 0;

	// We gradually build the value by appending each value by a multiplier
	// value which is equal to how many ms that unit represents:
	uint64_t in_ms = internal::MS_IN_SEC;
	ms += second * in_ms;
	in_ms *= internal::SECS_IN_MIN;
	ms += minute * in_ms;
	in_ms *= internal::MINS_IN_HR;
	ms += hour * in_ms;
	in_ms *= internal::HRS_IN_DAY;
	ms += (day - 1) * in_ms;

	// The month and year are more complicated. Months and years vary
	// depending on which years were leap years. We use lookup tables to
	// determine the correct value to use:

	// Month.
	if (internal::is_leap(year))
		ms += internal::DAY_OFFSET_BY_MONTH_LEAP[month - 1] * in_ms;
	else
		ms += internal::DAY_OFFSET_BY_MONTH_NO_LEAP[month - 1] * in_ms;
	// Year.
	ms += internal::DAY_OFFSET_BY_YEAR_SINCE_EPOCH[year - internal::MIN_YEAR] * in_ms;

	return ms;
}
} // namespace janus
