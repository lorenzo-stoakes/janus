#pragma once
#include <array>
#include <cstdint>
#include <stdexcept>
#include <string>

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
	static constexpr int64_t MIN = -1'152'921'504'606'846'976;
	static constexpr int64_t MAX = 1'152'921'504'606'846'975;

	// Default to 0.
	decimal7() : decimal7(0, 0) {}

	// Encode a value as decimal.
	//    val: The integer representation of the full decimal number without
	//         leading zeroes. E.g. for 1.234 this would be 1234.
	// places: The number of decimal places, e.g. for 1.234, this would be 3.
	decimal7(int64_t val, uint8_t places) // NOLINT: encoded _is_ initialised.
	{
		normalise(val, places);
		set(val, places);
	}

	// Equal to operator.
	auto operator==(const decimal7& that) const -> bool
	{
		return _encoded == that._encoded;
	}

	// Not equal to operator.
	auto operator!=(const decimal7& that) const -> bool
	{
		return !operator==(that);
	}

	// Number of decimal places.
	auto num_places() const -> uint8_t
	{
		return (_encoded & PLACE_MASK) >> VALUE_BITS;
	}

	// Integer part of value.
	auto int64() const -> int64_t
	{
		return raw() / exp10();
	}

	// Fractional part of value.
	// It's important to note here that fract() will strip leading zeroes
	// after the decimal point, e.g. 0.0003 wlll return fract() == 3.
	auto fract() const -> uint64_t
	{
		int64_t n = raw();
		return (n < 0 ? -n : n) % exp10();
	}

	// Convert to a double.
	auto to_double() const -> double
	{
		return static_cast<double>(raw()) / exp10();
	}

	// Returns the raw integral value (equivalent of removing the decimal
	// point) as if the number of places was now equal to req_places, i.e.
	// floor(floating_point_value * 10^req_places).
	//
	// E.g. for value 1.234:
	//   .mult10n(2) = 123
	//   .mult10n(4) = 1234
	//   .mult10n(6) = 123400
	//
	// If req_places > MAX_PLACES then an exception is thrown.
	auto mult10n(uint8_t req_places) const -> int64_t
	{
		if (req_places > MAX_PLACES)
			throw std::runtime_error(
				"Requested places " + std::to_string(static_cast<int>(req_places)) +
				" exceeds maximum of " + std::to_string(MAX_PLACES));

		return mult10n_unsafe(req_places);
	}

	// Returns raw integral value with 2 decimal places, e.g. 1.23 -> 123.
	// Equivalent to .mult10n(2).
	auto mult100() const -> int64_t
	{
		return mult10n_unsafe(2);
	}

	// Returns raw integral value with 3 decimal places, e.g. 1.234 -> 1234.
	// Equivalent to .mult10n(3).
	auto mult1000() const -> int64_t
	{
		return mult10n_unsafe(3);
	}

	// Returns the raw integral value (equivalent of removing the decimal
	// point), e.g. for 1.234 this would return 1234.
	// External callers probably don't need this.
	auto raw() const -> int64_t
	{
		uint64_t masked = _encoded & VALUE_MASK;
		// If this is a negative number we need to set the place bits
		// as in 2's complement these would only be clear for a very
		// large negative number.
		if ((masked & SIGN_MASK) == SIGN_MASK)
			masked |= PLACE_MASK;

		return reinterpret_cast<int64_t&>(masked); // NOLINT
	}

	// Returns the raw value including number of places encoding. Probably
	// nobody but a unit test will want this.
	auto raw_encoded() const -> uint64_t
	{
		return _encoded;
	}

private:
	static constexpr int64_t BASE = 10;
	static constexpr uint64_t PLACE_BITS = 3;
	static constexpr uint64_t VALUE_BITS = 63 - PLACE_BITS;
	static constexpr uint64_t MAX_PLACES = (1ULL << PLACE_BITS) - 1;
	static constexpr uint64_t SIGN_MASK = 1ULL << 63;
	static constexpr uint64_t VALUE_MASK = SIGN_MASK + (1ULL << VALUE_BITS) - 1;
	static constexpr uint64_t PLACE_MASK = ~VALUE_MASK;
	static constexpr std::array<int64_t, MAX_PLACES + 1> POW10S = {
		1, 10, 100, 1000, 10'000, 100'000, 1000'000, 10'000'000,
	};

	// The entirety of the data this type stores - a 64-bit value.
	uint64_t _encoded;

	// Returns 10^num_places.
	auto exp10() const -> int64_t
	{
		return POW10S[num_places()];
	}

	// Normalise specified value and number of decimal places such that
	// trailing zeroes are eliminated and places <= MAX_PLACES.
	static void normalise(int64_t& val, uint8_t& places)
	{
		// Firstly, constrain the value to the maximum representable
		// number of decimal places. E.g. 1.234 constrained to 2 places
		// would be encoded as 1.23.
		//
		// Secondly, elminate redundant trailing zeroes - e.g. (1230, 2)
		// and (123, 1) are equivalent. It is redundant to store the
		// trailing zeroes and doing so breaks equality checks. This
		// will also normalise zero values with places > 0 to (0, 0).
		while (places > MAX_PLACES || (places > 0 && val % BASE == 0)) {
			val /= BASE;
			places--;
		}
	}

	// Returns the raw integral value (equivalent of removing the decimal
	// point) as if the number of places was now equal to req_places, i.e.
	// floor(floating_point_value * 10^req_places).
	auto mult10n_unsafe(uint8_t req_places) const -> int64_t
	{
		uint8_t places = num_places();
		if (req_places == places)
			return raw();
		if (req_places > places)
			return raw() * POW10S[req_places - places];
		// req_places < places
		return raw() / POW10S[places - req_places];
	}

	// Encode value and places and set raw value.
	void set(int64_t val, uint8_t places)
	{
		_encoded = reinterpret_cast<uint64_t&>(val) & VALUE_MASK; // NOLINT
		_encoded |= (static_cast<uint64_t>(places) << VALUE_BITS) & PLACE_MASK;
	}
};
} // namespace janus
