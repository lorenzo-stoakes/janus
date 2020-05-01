#include "janus.hh"

#include <cstdlib>
#include <cstring>
#include <ctime>
#include <gtest/gtest.h>
#include <iomanip>
#include <sstream>
#include <string>
#include <string_view>

namespace
{
// Helper method to get epoch time using system libraries.
uint64_t get_epoch_ms(uint64_t year, uint64_t month, uint64_t day, uint64_t hour, uint64_t min,
		      uint64_t sec, uint64_t ms, std::string& as_str)
{
	// Pretty Heath Robinson.
	std::ostringstream oss;
	oss << year << "-" << std::setw(2) << std::setfill('0') << month << "-" << std::setw(2)
	    << day << "T" << std::setw(2) << hour << ":" << std::setw(2) << min << ":"
	    << std::setw(2) << sec;
	std::string str = oss.str();

	// Parse the time and obtain a 'tm' struct value.
	std::tm tm_time = {0};
	::strptime(str.c_str(), "%Y-%m-%dT%H:%M:%S", &tm_time);

	// Then obtain seconds since epoch in UTC.
	// Safe to assume time_t is a uint64.
	uint64_t secs_since_epoch = ::timegm(&tm_time);

	// Convert to ISO-8601 string.
	as_str = str + "." + std::to_string(ms) + "Z";

	return secs_since_epoch * 1000 + ms;
}

// Helper method to convert ms value to days.
auto ms_to_days(uint64_t ms) -> uint64_t
{
	uint64_t secs = ms / 1000;
	uint64_t minutes = secs / 60;
	uint64_t hours = minutes / 60;
	uint64_t days = hours / 24;

	return days;
}

// Test that the parse_digits() internal helper function correctly parses digits
// from 1 - 4 digits long.
TEST(parse_test, parse_digits)
{
	// Single digit values.
	for (uint64_t i = 0; i < 10; i++) {
		uint64_t actual = janus::internal::parse_digits<1>(std::to_string(i).c_str());
		EXPECT_EQ(i, actual);
	}

	// 2 digits.
	for (uint64_t i = 10; i < 100; i++) {
		uint64_t actual = janus::internal::parse_digits<10>(std::to_string(i).c_str());
		EXPECT_EQ(i, actual);
	}

	// 3 digits.
	for (uint64_t i = 100; i < 1000; i++) {
		uint64_t actual = janus::internal::parse_digits<100>(std::to_string(i).c_str());
		EXPECT_EQ(i, actual);
	}

	// 4 digits.
	for (uint64_t i = 1000; i < 10'000; i++) {
		uint64_t actual = janus::internal::parse_digits<1000>(std::to_string(i).c_str());
		EXPECT_EQ(i, actual);
	}
}

// Test that the is_leap() internal helper function correctly identifies leap years.
TEST(parse_test, is_leap)
{
	EXPECT_TRUE(janus::internal::is_leap(1600));
	EXPECT_FALSE(janus::internal::is_leap(1700));
	EXPECT_FALSE(janus::internal::is_leap(1800));
	EXPECT_FALSE(janus::internal::is_leap(1900));
	EXPECT_TRUE(janus::internal::is_leap(2000));
	EXPECT_TRUE(janus::internal::is_leap(2020));
}

// Test that parse_iso8601() correctly parses a date/time in the ISO-8601 format
// (e.g. 2020-03-11T13:20:00.123Z)
TEST(parse_test, parse_iso8601)
{
	auto parse = [&](std::string str) { return janus::parse_iso8601(str.c_str(), str.size()); };

	// Start with an arbitrary February 29th.
	std::string timestamp_str;
	uint64_t expected = get_epoch_ms(2020, 2, 29, 17, 39, 57, 176, timestamp_str);
	EXPECT_EQ(parse(timestamp_str), expected) << "Couldn't parse " << timestamp_str;

	// Now range over some days that occur in every year.
	for (uint64_t year = 1970; year <= janus::internal::MAX_YEAR; year++) {
		for (uint64_t month = 1; month <= 12; month++) {
			for (uint64_t day = 1; day <= 28; day++) {
				// Check 12:34:56.789 for each day.
				expected = get_epoch_ms(year, month, day, 12, 34, 56, 789,
							timestamp_str);
				ASSERT_EQ(parse(timestamp_str), expected)
					<< "Couldn't parse " << timestamp_str;

				ASSERT_EQ(janus::encode_epoch(year, month, day, 12, 34, 56, 789),
					  expected);

				ASSERT_EQ(janus::encode_epoch_days(year, month, day),
					  ms_to_days(expected));

				expected -= 789;
				std::string without_ms_str =
					timestamp_str.substr(0, timestamp_str.size() - 5) + "Z";
				// Check 12:34:56 for each day.
				ASSERT_EQ(parse(without_ms_str), expected)
					<< "Couldn't parse " << without_ms_str;
			}
		}
	}

	// Now range over times on a fixed date.
	for (uint64_t hour = 0; hour < 24; hour++) {
		for (uint64_t min = 0; min < 60; min++) {
			for (uint64_t sec = 0; sec < 60; sec++) {
				// Sticking to .789 ms.
				expected = get_epoch_ms(2020, 4, 4, hour, min, sec, 789,
							timestamp_str);
				ASSERT_EQ(parse(timestamp_str), expected)
					<< "Couldn't parse " << timestamp_str;

				ASSERT_EQ(janus::encode_epoch(2020, 4, 4, hour, min, sec, 789),
					  expected);

				expected -= 789;
				std::string without_ms_str =
					timestamp_str.substr(0, timestamp_str.size() - 5) + "Z";
				// Check 12:34:56 for each day.
				ASSERT_EQ(parse(without_ms_str), expected)
					<< "Couldn't parse " << without_ms_str;
			}
		}
	}

	// Now try out some invalid inputs.
	EXPECT_EQ(parse(""), 0);
	EXPECT_EQ(parse("ohai"), 0);
	EXPECT_EQ(parse("2020-03-11T13:20:00.123"), 0);
	EXPECT_EQ(parse("2020-03-11T13:20:00."), 0);
	EXPECT_EQ(parse("2020-03-11T13:20:00"), 0);
	EXPECT_EQ(parse("2020-03-11T13:20:0"), 0);
	EXPECT_EQ(parse("2020-03-11T13:20:00.123z"), 0);
	EXPECT_EQ(parse("2020-03-11T13:20:00.123x"), 0);
}

// Test that internal method .print_digits() prints digits correctly to a char
// buffer.
TEST(parse_test, print_digits)
{
	char buf[5] = {0};

	// Single digit values.
	for (uint64_t i = 0; i < 10; i++) {
		char* ptr = janus::internal::print_digits<1>(buf, i);
		ASSERT_EQ(buf[0], '0' + i);
		ASSERT_EQ(ptr, &buf[1]);
	}

	// 2 digits.
	for (uint64_t i = 0; i < 100; i++) {
		char* ptr = janus::internal::print_digits<10>(buf, i);
		if (i < 10)
			ASSERT_EQ(buf[0], '0');
		else
			ASSERT_EQ(buf[0], '0' + (i / 10));
		ASSERT_EQ(buf[1], '0' + (i % 10));
		ASSERT_EQ(ptr, &buf[2]);
	}

	// 3 digits.
	for (uint64_t i = 0; i < 1000; i++) {
		char* ptr = janus::internal::print_digits<100>(buf, i);
		if (i < 100)
			ASSERT_EQ(buf[0], '0');
		else
			ASSERT_EQ(buf[0], '0' + (i / 100));

		if (i < 10)
			ASSERT_EQ(buf[1], '0');
		else
			ASSERT_EQ(buf[1], '0' + ((i / 10) % 10));
		ASSERT_EQ(buf[2], '0' + (i % 10));
		ASSERT_EQ(ptr, &buf[3]);
	}

	// 4 digits.
	for (uint64_t i = 100; i < 10'000; i++) {
		char* ptr = janus::internal::print_digits<1000>(buf, i);
		if (i < 1000)
			ASSERT_EQ(buf[0], '0');
		else
			ASSERT_EQ(buf[0], '0' + (i / 1000));

		if (i < 100)
			ASSERT_EQ(buf[1], '0');
		else
			ASSERT_EQ(buf[1], '0' + ((i / 100) % 10));

		if (i < 10)
			ASSERT_EQ(buf[2], '0');
		else
			ASSERT_EQ(buf[2], '0' + ((i / 10) % 10));
		ASSERT_EQ(buf[3], '0' + (i % 10));
		ASSERT_EQ(ptr, &buf[4]);
	}
}

// Test that .print_iso8601() method correctly outputs ISO-8601 numbers to a
// character buffer from ms since epoch values.
TEST(parse_test, print_iso8601)
{
	char buf[25];

	// Parse value then print it back and make sure they match.

	const char* test_timestamp = "2020-03-11T13:20:00.123Z";
	uint64_t epoch_ms = janus::parse_iso8601(test_timestamp, 24);
	std::string_view view = janus::print_iso8601(buf, epoch_ms);

	EXPECT_EQ(std::strcmp(buf, test_timestamp), 0)
		<< "Expected " << test_timestamp << " actual " << buf;

	// Make sure returned string view reflects what was written to buf.
	EXPECT_EQ(std::strcmp(view.data(), buf), 0) << "Expected " << buf << " actual " << view;

	// Now range over some days that occur in every year.
	std::string timestamp_str;
	for (uint64_t year = 1970; year <= janus::internal::MAX_YEAR; year++) {
		for (uint64_t month = 1; month <= 12; month++) {
			for (uint64_t day = 1; day <= 28; day++) {
				// Check 12:34:56.789 for each day.
				epoch_ms = get_epoch_ms(year, month, day, 12, 34, 56, 789,
							timestamp_str);

				uint64_t actual_year;
				uint64_t actual_month;
				uint64_t actual_day;
				uint64_t actual_hour;
				uint64_t actual_minute;
				uint64_t actual_second;
				uint64_t actual_ms;
				janus::unpack_epoch_ms(epoch_ms, actual_year, actual_month,
						       actual_day, actual_hour, actual_minute,
						       actual_second, actual_ms);
				EXPECT_EQ(actual_year, year);
				EXPECT_EQ(actual_month, month);
				EXPECT_EQ(actual_day, day);
				EXPECT_EQ(actual_hour, 12);
				EXPECT_EQ(actual_minute, 34);
				EXPECT_EQ(actual_second, 56);
				EXPECT_EQ(actual_ms, 789);

				view = janus::print_iso8601(buf, epoch_ms);
				EXPECT_EQ(std::strcmp(buf, timestamp_str.c_str()), 0);
				EXPECT_EQ(std::strcmp(view.data(), timestamp_str.c_str()), 0);
			}
		}
	}

	// Now range over times on a fixed date.
	for (uint64_t hour = 0; hour < 24; hour++) {
		for (uint64_t min = 0; min < 60; min++) {
			for (uint64_t sec = 0; sec < 60; sec++) {
				// Sticking to .789 ms.
				epoch_ms = get_epoch_ms(2020, 4, 4, hour, min, sec, 789,
							timestamp_str);

				uint64_t actual_year;
				uint64_t actual_month;
				uint64_t actual_day;
				uint64_t actual_hour;
				uint64_t actual_minute;
				uint64_t actual_second;
				uint64_t actual_ms;
				janus::unpack_epoch_ms(epoch_ms, actual_year, actual_month,
						       actual_day, actual_hour, actual_minute,
						       actual_second, actual_ms);
				EXPECT_EQ(actual_year, 2020);
				EXPECT_EQ(actual_month, 4);
				EXPECT_EQ(actual_day, 4);
				EXPECT_EQ(actual_hour, hour);
				EXPECT_EQ(actual_minute, min);
				EXPECT_EQ(actual_second, sec);
				EXPECT_EQ(actual_ms, 789);

				view = janus::print_iso8601(buf, epoch_ms);
				EXPECT_EQ(std::strcmp(buf, timestamp_str.c_str()), 0);
				EXPECT_EQ(std::strcmp(view.data(), timestamp_str.c_str()), 0);
			}
		}
	}
}

// Ensure that internal function parse_market_id() correctly parses betfair market IDs.
TEST(parse_test, parse_market_id)
{
	// Errors result in 0 being returned.
	EXPECT_EQ(janus::internal::parse_market_id("", 0), 0);

	EXPECT_EQ(janus::internal::parse_market_id("1.1", 3), 1);
	EXPECT_EQ(janus::internal::parse_market_id("1.12", 4), 12);
	EXPECT_EQ(janus::internal::parse_market_id("1.123", 5), 123);
	EXPECT_EQ(janus::internal::parse_market_id("1.1234", 6), 1234);

	// Try a long number.
	EXPECT_EQ(janus::internal::parse_market_id("1.1234567890123", 17), 1'234'567'890'123ULL);
}
} // namespace
