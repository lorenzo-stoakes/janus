#include "janus.hh"

#include <array>
#include <cstdint>
#include <string_view>

namespace janus
{
namespace internal
{
// Lookup tables.
static const std::array<uint64_t, 13> DAY_OFFSET_BY_MONTH_NO_LEAP = {
	0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334, 365};
static const std::array<uint64_t, 13> DAY_OFFSET_BY_MONTH_LEAP = {0,   31,  60,  91,  121, 152, 182,
								  213, 244, 274, 305, 335, 366};
static const std::array<uint64_t, NUM_YEARS + 1> DAY_OFFSET_BY_YEAR_SINCE_EPOCH = {
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
	43830, 44195, 44560, 44926, 45291, 45656, 46021, 46387, 46752, 47117, 47482, 47847};

// Convenient units.
static constexpr uint64_t MS_IN_SEC = 1000;
static constexpr uint64_t SECS_IN_MIN = 60;
static constexpr uint64_t MINS_IN_HR = 60;
static constexpr uint64_t HRS_IN_DAY = 24;
static constexpr uint64_t DAYS_IN_YEAR = 365;
static constexpr uint64_t MONTHS_IN_YEAR = 12;

// Some useful aliases.
static inline auto parse_digits10 = parse_digits<10>;     // NOLINT: Not magical.
static inline auto parse_digits100 = parse_digits<100>;   // NOLINT: Not magical.
static inline auto parse_digits1000 = parse_digits<1000>; // NOLINT: Not magical.

static inline auto print_digits10 = print_digits<10>;     // NOLINT: Not magical.
static inline auto print_digits100 = print_digits<100>;   // NOLINT: Not magical.
static inline auto print_digits1000 = print_digits<1000>; // NOLINT: Not magical.
} // namespace internal

auto parse_iso8601(const char* str, uint64_t size) -> uint64_t
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
	//      **
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

void unpack_epoch_ms(uint64_t epoch_ms, uint64_t& year, uint64_t& month, uint64_t& day,
		     uint64_t& hour, uint64_t& minute, uint64_t& second, uint64_t& ms)
{
	ms = epoch_ms % internal::MS_IN_SEC;
	epoch_ms /= internal::MS_IN_SEC;
	second = epoch_ms % internal::SECS_IN_MIN;
	epoch_ms /= internal::SECS_IN_MIN;
	minute = epoch_ms % internal::MINS_IN_HR;
	epoch_ms /= internal::MINS_IN_HR;
	hour = epoch_ms % internal::HRS_IN_DAY;
	epoch_ms /= internal::HRS_IN_DAY;

	// We are now left with the number of days from epoch to 00:00 on the
	// date specified.
	uint64_t days_in_date = epoch_ms;

	// First guess which year we are in.
	year = internal::MIN_YEAR + days_in_date / internal::DAYS_IN_YEAR;

	// If we are wrong, we will appear to be in a later year than we
	// actually are due to leap years meaning we under-estimate the length
	// of a year(s).
	if (year > internal::MIN_YEAR) {
		// For a year to be out by more than 1 we would have had to have
		// >=365 Feb 29th's.
		uint64_t day_offset =
			internal::DAY_OFFSET_BY_YEAR_SINCE_EPOCH[year - internal::MIN_YEAR];
		if (days_in_date < day_offset)
			year--;
	}

	// Now extract the number of days we are offset from Jan 1st in the year.
	uint64_t days_in_year =
		days_in_date - internal::DAY_OFFSET_BY_YEAR_SINCE_EPOCH[year - internal::MIN_YEAR];

	// Linear search day offsets in month. This is faster than alternatives
	// due to cache friendliness.

	const auto& month_offsets = internal::is_leap(year) ? internal::DAY_OFFSET_BY_MONTH_LEAP
							    : internal::DAY_OFFSET_BY_MONTH_NO_LEAP;
	uint64_t month_offset = 0;

	// The month value is offset by 1 so is equal to the human-readable
	// month number.
	for (month = 1; month <= internal::MONTHS_IN_YEAR; month++) {
		if (days_in_year < month_offsets[month]) {
			month_offset = month_offsets[month - 1];
			break;
		}
	}

	// Retrieve day offset in month and offset by 1 for a human-readable day
	// number.
	day = days_in_year - month_offset + 1;
}

auto print_iso8601(char* str, uint64_t epoch_ms) -> std::string_view
{
	uint64_t year;
	uint64_t month;
	uint64_t day;
	uint64_t hour;
	uint64_t minute;
	uint64_t second;
	uint64_t ms;
	unpack_epoch_ms(epoch_ms, year, month, day, hour, minute, second, ms);

	char* ptr = str;

	ptr = internal::print_digits1000(ptr, year);
	*ptr++ = '-';
	ptr = internal::print_digits10(ptr, month);
	*ptr++ = '-';
	ptr = internal::print_digits10(ptr, day);
	*ptr++ = 'T';
	ptr = internal::print_digits10(ptr, hour);
	*ptr++ = ':';
	ptr = internal::print_digits10(ptr, minute);
	*ptr++ = ':';
	ptr = internal::print_digits10(ptr, second);
	*ptr++ = '.';
	ptr = internal::print_digits100(ptr, ms);
	*ptr++ = 'Z';

	// Null terminate string.
	*ptr = '\0';
	return std::string_view(str, 24); // NOLINT: Not magical.
}
} // namespace janus
