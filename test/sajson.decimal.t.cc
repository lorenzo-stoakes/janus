#include "janus.hh"
#include "sajson.hh"

#include <gtest/gtest.h>
#include <sstream>
#include <string>

namespace
{
// Stealing the idea from sajson::literal to make it more convenient to parse
// string literals.
template<uint64_t sz>
static const sajson::document parse(const char (&text)[sz])
{
	return sajson::parse(
		// The original tests already heavily cover different allocation
		// strategies including with decimal values, so it is reasonable
		// to simply stick to one here.
		sajson::single_allocation(), sajson::string(text, sz - 1));
}

// Helper function to check document and extract error message if it fails for
// easier debug.
static const std::string check_doc(const sajson::document& doc)
{
	if (doc.is_valid())
		return "";

	uint64_t line = doc.get_error_line();
	uint64_t col = doc.get_error_column();
	std::string msg = doc.get_error_message_as_string();

	std::ostringstream oss;
	oss << line << ":" << col << ": " << msg;
	return oss.str();
}

// Basic 'does this work at all' tests.
TEST(sajson_decimal_test, basic)
{
	const sajson::document doc = parse("[1.23, -1.23]");
	EXPECT_EQ(check_doc(doc), "");

	const sajson::value& val1 = doc.get_root().get_array_element(0);
	EXPECT_EQ(val1.get_type(), sajson::TYPE_DECIMAL);
	EXPECT_EQ(val1.get_decimal_value(), janus::decimal7(123, 2));

	const sajson::value& val2 = doc.get_root().get_array_element(1);
	EXPECT_EQ(val2.get_type(), sajson::TYPE_DECIMAL);
	EXPECT_EQ(val2.get_decimal_value(), janus::decimal7(-123, 2));
}

// Ensure that .get_number_value() behaves as expected.
TEST(sajson_decimal_test, get_number_value)
{
	const sajson::document doc = parse("[1.23, -1.23, 123, -123, -1e3, 1e3]");
	EXPECT_EQ(check_doc(doc), "");

	const sajson::value& val1 = doc.get_root().get_array_element(0);
	EXPECT_EQ(val1.get_type(), sajson::TYPE_DECIMAL);
	EXPECT_DOUBLE_EQ(val1.get_number_value(), 1.23);

	const sajson::value& val2 = doc.get_root().get_array_element(1);
	EXPECT_EQ(val2.get_type(), sajson::TYPE_DECIMAL);
	EXPECT_DOUBLE_EQ(val2.get_number_value(), -1.23);

	// Make sure we don't somehow make integers behave like decimals...

	const sajson::value& val3 = doc.get_root().get_array_element(2);
	EXPECT_EQ(val3.get_type(), sajson::TYPE_INTEGER);
	EXPECT_DOUBLE_EQ(val3.get_number_value(), 123);

	const sajson::value& val4 = doc.get_root().get_array_element(3);
	EXPECT_EQ(val4.get_type(), sajson::TYPE_INTEGER);
	EXPECT_DOUBLE_EQ(val4.get_number_value(), -123);

	// Exponents are an edge case - Really we should be able to treat these
	// as integers or decimals, but this is just not implemented yet.

	const sajson::value& val5 = doc.get_root().get_array_element(4);
	EXPECT_EQ(val5.get_type(), sajson::TYPE_DOUBLE);
	EXPECT_DOUBLE_EQ(val5.get_number_value(), -1000);

	const sajson::value& val6 = doc.get_root().get_array_element(5);
	EXPECT_EQ(val6.get_type(), sajson::TYPE_DOUBLE);
	EXPECT_DOUBLE_EQ(val6.get_number_value(), 1000);
}

// Test that behaviour is as expected at minimum/maximum representable decimal
// values, as well as values that exceed these.
TEST(sajson_decimal_test, limits)
{
	const sajson::document doc = parse("[-1152921504606846976, 1152921504606846975,"
					   "-115292150460.6846976, 115292150460.6846975,"
					   "-115292150460.6846977, 115292150460.6846976]");
	EXPECT_EQ(check_doc(doc), "");

	const sajson::value& val1 = doc.get_root().get_array_element(0);
	EXPECT_EQ(val1.get_type(), sajson::TYPE_INTEGER);
	EXPECT_EQ(val1.get_decimal_value(), janus::decimal7(-1152921504606846976, 0));

	const sajson::value& val2 = doc.get_root().get_array_element(1);
	EXPECT_EQ(val2.get_type(), sajson::TYPE_INTEGER);
	EXPECT_EQ(val2.get_decimal_value(), janus::decimal7(1152921504606846975, 0));

	const sajson::value& val3 = doc.get_root().get_array_element(2);
	EXPECT_EQ(val3.get_type(), sajson::TYPE_DECIMAL);
	EXPECT_EQ(val3.get_decimal_value(), janus::decimal7(-1152921504606846976, 7));

	const sajson::value& val4 = doc.get_root().get_array_element(3);
	EXPECT_EQ(val4.get_type(), sajson::TYPE_DECIMAL);
	EXPECT_EQ(val4.get_decimal_value(), janus::decimal7(1152921504606846975, 7));

	const sajson::value& val5 = doc.get_root().get_array_element(4);
	EXPECT_EQ(val5.get_type(), sajson::TYPE_DOUBLE);

	const sajson::value& val6 = doc.get_root().get_array_element(5);
	EXPECT_EQ(val6.get_type(), sajson::TYPE_DOUBLE);
}

// Exponents are not converted into decimal. This is something that I may revisit.
TEST(sajson_decimal_test, exponents)
{
	const sajson::document doc = parse("[-1e-3, 1e-3]");
	EXPECT_EQ(check_doc(doc), "");

	const sajson::value& val1 = doc.get_root().get_array_element(0);
	EXPECT_EQ(val1.get_type(), sajson::TYPE_DOUBLE);
	EXPECT_DOUBLE_EQ(val1.get_double_value(), -0.001);

	const sajson::value& val2 = doc.get_root().get_array_element(1);
	EXPECT_EQ(val2.get_type(), sajson::TYPE_DOUBLE);
	EXPECT_DOUBLE_EQ(val2.get_double_value(), 0.001);
}

// Assert what happens when we have more decimal places than can be represented.
TEST(sajson_decimal_test, too_many_decimal_places)
{
	const sajson::document doc = parse("[-1234.123456789, 1234.123456789]");
	EXPECT_EQ(check_doc(doc), "");

	// These are still decimal values, we just truncate to 7 decimal places.

	const sajson::value& val1 = doc.get_root().get_array_element(0);
	EXPECT_EQ(val1.get_type(), sajson::TYPE_DECIMAL);
	EXPECT_EQ(val1.get_decimal_value(), janus::decimal7(-12341234567LL, 7));

	const sajson::value& val2 = doc.get_root().get_array_element(1);
	EXPECT_EQ(val2.get_type(), sajson::TYPE_DECIMAL);
	EXPECT_EQ(val2.get_decimal_value(), janus::decimal7(12341234567LL, 7));
}

// Decimal values should be convertible to double in .get_double_value()
TEST(sajson_decimal_test, get_double_value)
{
	const sajson::document doc = parse("[-1.23, 1.23]");
	EXPECT_EQ(check_doc(doc), "");

	const sajson::value& val1 = doc.get_root().get_array_element(0);
	EXPECT_EQ(val1.get_type(), sajson::TYPE_DECIMAL);
	EXPECT_DOUBLE_EQ(val1.get_double_value(), -1.23);

	const sajson::value& val2 = doc.get_root().get_array_element(1);
	EXPECT_EQ(val2.get_type(), sajson::TYPE_DECIMAL);
	EXPECT_DOUBLE_EQ(val2.get_double_value(), 1.23);
}

// Integers should be convertible to decimal via .get_decimal_value()
TEST(sajson_decimal_test, integer_get_decimal_value)
{
	const sajson::document doc = parse("[-123, 123]");
	EXPECT_EQ(check_doc(doc), "");

	const sajson::value& val1 = doc.get_root().get_array_element(0);
	EXPECT_EQ(val1.get_type(), sajson::TYPE_INTEGER);
	EXPECT_EQ(val1.get_decimal_value(), janus::decimal7(-123, 0));

	const sajson::value& val2 = doc.get_root().get_array_element(1);
	EXPECT_EQ(val2.get_type(), sajson::TYPE_INTEGER);
	EXPECT_EQ(val2.get_decimal_value(), janus::decimal7(123, 0));
}
} // namespace
