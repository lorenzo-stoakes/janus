#pragma once

#include "dynamic_array.hh"
#include "market.hh"

#include <array>
#include <cstdint>
#include <stdexcept>
#include <string>

namespace janus::betfair
{
// Represents a 'universe' of markets. It has a fixed capacity which is
// specified as a template parameter.
template<uint64_t Cap>
class universe
{
public:
	using markets_t = dynamic_array<market, Cap>;

	universe() : _num_markets{0}, _market_ids{0} {}

	// Get MUTABLE reference to markets in universe.
	auto markets() -> markets_t&
	{
		return _markets;
	}

	// Get MUTABLE reference to market with specified ID. Market MUST exist
	// or this will null pointer deref!
	auto operator[](uint64_t id) -> market&
	{
		return *get_market(id);
	}

	// Get MUTABLE pointer to market with specified ID. Returns nullptr if
	// not found.
	auto find_market(uint64_t id) -> market*
	{
		return get_market(id);
	}

	// Determine whether market with specified Id exists in universe.
	auto contains_market(uint64_t id) -> bool
	{
		return get_market(id) != nullptr;
	}

	// Get the number of markets contained in the universe.
	auto num_markets() -> uint64_t
	{
		return _num_markets;
	}

	// Create a new market with the specific ID.
	void add_market(uint64_t id)
	{
		if (_num_markets == Cap)
			throw std::runtime_error("Adding market would exceed capacity of " +
						 std::to_string(Cap));

		_market_ids[_num_markets++] = id;
		_markets.emplace_back(id);
	}

private:
	uint64_t _num_markets;
	std::array<uint64_t, Cap> _market_ids;
	markets_t _markets;

	// Get pointer to market with specified ID. Returns nullptr if not
	// found.
	auto get_market(uint64_t id) -> market*
	{
		// For low N this is faster than hashing.
		for (uint64_t i = 0; i < _num_markets; i++) {
			if (_market_ids[i] == id)
				return &_markets[i];
		}

		return nullptr;
	}
};
} // namespace janus::betfair
