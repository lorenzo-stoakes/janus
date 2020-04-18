#pragma once

#include "dynamic_array.hh"
#include "market.hh"
#include "runner.hh"
#include "update.hh"

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

	universe()
		: _num_markets{0},
		  _last_timestamp{0},
		  _num_updates{0},
		  _last_market{nullptr},
		  _last_runner{nullptr},
		  _market_ids{0}
	{
	}

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
	auto num_markets() const -> uint64_t
	{
		return _num_markets;
	}

	// Get last timestamp universe was updated at.
	auto last_timestamp() const -> uint64_t
	{
		return _last_timestamp;
	}

	// Get number of updates this universe has received.
	auto num_updates() const -> uint64_t
	{
		return _num_updates;
	}

	// Get the last market to have an update applied to it, or nullptr if no
	// market updated yet.
	auto last_market() -> market*
	{
		return _last_market;
	}

	// Get the last runner to have an update applied to it, or nullptr if no
	// runner updated yet.
	auto last_runner() -> runner*
	{
		return _last_runner;
	}

	// Set last timestamp universe was updated at.
	void set_last_timestamp(uint64_t timestamp)
	{
		_last_timestamp = timestamp;
	}

	// Create a new market with the specific ID.
	auto add_market(uint64_t id) -> market&
	{
		if (_num_markets == Cap)
			throw std::runtime_error("Adding market would exceed capacity of " +
						 std::to_string(Cap));

		_market_ids[_num_markets++] = id;
		return _markets.emplace_back(id);
	}

	// Apply update to universe, this might create a market, runner, or
	// update an existing market/runner.
	void apply_update(const update& update);

private:
	uint64_t _num_markets;
	uint64_t _last_timestamp;
	uint64_t _num_updates;
	market* _last_market;
	runner* _last_runner;
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

	// Internal functions implementing apply_update().

	// Apply market ID update.
	void apply_market_id(uint64_t id);

	// Apply runner ID update.
	void apply_runner_id(uint64_t id);

	// Apply timestamp update (to universe specifically).
	void apply_timestamp(uint64_t timestamp);

	// Apply clear of market, clearing mutable values in market, runners and
	// ladders.
	void apply_market_clear();

	// Apply market state.
	void apply_market_state(market_state state);

	// Apply market inplay, setting market inplay to true.
	void apply_market_inplay();

	// Apply market traded volume.
	void apply_market_traded_vol(double vol);

	// Apply runner removal.
	void apply_runner_removal(double adj_factor);

	// Apply runner removal.
	void apply_runner_traded_vol(double vol);

	// Apply runner LTP.
	void apply_runner_ltp(uint64_t price_index);

	// ivg

	// Apply runner SP.
	void apply_runner_sp(double sp);

	// Apply runner won state - mark it as won.
	void apply_runner_won();

	// Apply runner matched data at specified price index.
	void apply_runner_matched(uint64_t price_index, double vol);

	// Apply runner unmatched data at specified price index, positive volume
	// indicates ATL, negative ATB.
	void apply_runner_unmatched(uint64_t price_index, double vol);

	// Set market timestamp to universe timestamp - invoked when
	// market/runner update occurs.
	void set_market_timestamp();

	// Set runner timestamp to universe timestamp - invoked when runner
	// update occurs.
	void set_runner_timestamp();
};
} // namespace janus::betfair

#include "universe.tcc"
