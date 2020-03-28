#pragma once
#include <cstdint>

namespace janus
{
// 64-bit representation of a floating point DECIMAL number with up to 7 decimal
// places. All digits are stored integrally.
//
// See https://en.wikipedia.org/wiki/Decimal_data_type
//
// For example, 1.234 would be stored as 1234 with a places value of 3, and
// 0.0003 would be stored as 3 with a places value of 4.
//
// The number is binary encoded as:
//      6         5         4         3         2         1
//   3210987654321098765432109876543210987654321098765432109876543210
//   spppnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnn
//
//   s = sign bit
//   p = place bits
//   n = value bits
//
// The place bits overwrite the upper bits of the value, reducing the maximum
// value by 3 bits.
//
// The largest positive and negative numbers are:
// 0b0000111111111111111111111111111111111111111111111111111111111111
// 0b1111000000000000000000000000000000000000000000000000000000000000
//
// Largest integral number     =  1'152'921'504'606'846'975
// Largest -ve integral nubmer = -1'152'921'504'606'846'976
// Largest number with 7dp     =            115'292'150'460.6846975
class decimal7
{
public:
	// Default to 0.
	decimal7() : decimal7(0, 0) {}

	// Encode a value as decimal.
	//    val: The integer representation of the full decimal number without
	//         leading zeroes. E.g. for 1.234 this would be 1234.
	// places: The number of decimal places, e.g. for 1.234, this would be 3.
	decimal7(int64_t val, uint8_t places)
	{
		normalise(val, places);
		set(val, places);
	}

	// Equal to operator.
	bool operator==(const decimal7& that) const
	{
		return encoded == that.encoded;
	}

	// Not equal to operator.
	bool operator!=(const decimal7& that) const
	{
		return !operator==(that);
	}

	// Number of decimal places.
	uint8_t num_places() const
	{
		return (encoded & PLACE_MASK) >> VALUE_BITS;
	}

	// Integer part of value.
	int64_t int64() const
	{
		return raw() / exp10();
	}

	// Fractional part of value.
	// It's important to note here that fract() will strip leading zeroes
	// after the decimal point, e.g. 0.0003 wlll return fract() == 3.
	uint64_t fract() const
	{
		int64_t n = raw();
		return (n < 0 ? -n : n) % exp10();
	}

	// Convert to a double.
	double to_double() const
	{
		return static_cast<double>(raw()) / exp10();
	}

	// Returns the raw integral value (equivalent of removing the decimal
	// point) as if the number of places was now equal to req_places, i.e.
	// floor(floating_point_value * 10^req_places).
	//
	// E.g. for value 1.234:
	//   .mult10(2) = 123
	//   .mult10(4) = 1234
	//   .mult10(6) = 123400
	//
	// If req_places > MAX_PLACES then this returns .mult10(MAX_PLACES).
	// This would probably be unexpected behaviour for a caller, so best to
	// ensure that req_places <= MAX_PLACES.
	int64_t mult10(uint8_t req_places) const
	{
		// Constrain to max places.
		req_places &= MAX_PLACES;

		uint8_t places = num_places();
		if (req_places == places)
			return raw();
		else if (req_places > places)
			return raw() * POW10S[req_places - places];
		else // req_places < places
			return raw() / POW10S[places - req_places];
	}

	// Returns the raw integral value (equivalent of removing the decimal
	// point), e.g. for 1.234 this would return 1234.
	// External callers probably don't need this.
	int64_t raw() const
	{
		uint64_t masked = encoded & VALUE_MASK;
		// If this is a negative number we need to set the place bits
		// as in 2's complement these would only be clear for a very
		// large negative number.
		if (masked & SIGN_MASK)
			masked |= PLACE_MASK;

		return reinterpret_cast<int64_t&>(masked);
	}

	// Returns the raw value including number of places encoding. Probably
	// nobody but a unit test will want this.
	uint64_t raw_encoded() const
	{
		return encoded;
	}

private:
	static constexpr uint64_t PLACE_BITS = 3;
	static constexpr uint64_t VALUE_BITS = 63 - PLACE_BITS;
	static constexpr uint64_t MAX_PLACES = (1ULL << PLACE_BITS) - 1;
	static constexpr uint64_t SIGN_MASK = 1ULL << 63;
	static constexpr uint64_t VALUE_MASK = SIGN_MASK + (1ULL << VALUE_BITS) - 1;
	static constexpr uint64_t PLACE_MASK = ~VALUE_MASK;
	static constexpr int64_t POW10S[MAX_PLACES + 1] = {
		1, 10, 100, 1000, 10'000, 100'000, 1000'000, 10'000'000,
	};

	// The entirety of the data this type stores - a 64-bit value.
	uint64_t encoded;

	// Returns 10^num_places.
	int64_t exp10() const
	{
		return POW10S[num_places()];
	}

	// Normalise specified value and number of decimal places such that
	// trailing zeroes are eliminated and places <= MAX_PLACES.
	void normalise(int64_t& val, uint8_t& places) const
	{
		// Firstly, constrain the value to the maximum representable
		// number of decimal places. E.g. 1.234 constrained to 2 places
		// would be encoded as 1.23.
		//
		// Secondly, elminate redundant trailing zeroes - e.g. (1230, 2)
		// and (123, 1) are equivalent. It is redundant to store the
		// trailing zeroes and doing so breaks equality checks. This
		// will also normalise zero values with places > 0 to (0, 0).
		while (places > MAX_PLACES || (places > 0 && val % 10 == 0)) {
			val /= 10;
			places--;
		}
	}

	// Encode value and places and set raw value.
	void set(int64_t val, uint8_t places)
	{
		encoded = reinterpret_cast<uint64_t&>(val) & VALUE_MASK;
		encoded |= (static_cast<uint64_t>(places) << VALUE_BITS) & PLACE_MASK;
	}
};
} // namespace janus
