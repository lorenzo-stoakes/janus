#pragma once

#include "dynamic_array.hh"
#include "runner.hh"

#include <array>
#include <cstdint>
#include <stdexcept>
#include <string>

namespace janus::betfair
{
constexpr uint64_t MAX_RUNNERS = 50;

// The current operational state of the market.
enum class market_state : uint64_t
{
	OPEN,
	CLOSED,
	SUSPENDED,
};

class market
{
public:
	using runners_t = dynamic_array<runner, MAX_RUNNERS>;

	explicit market(uint64_t id)
		: _id{id},
		  _state{market_state::OPEN},
		  _traded_vol{0},
		  _num_runners{0},
		  _last_timestamp{0},
		  _runner_ids{0}
	{
	}

	// Get the ID of this market.
	auto id() const -> uint64_t
	{
		return _id;
	}

	// Get the current state of this market.
	auto state() const -> market_state
	{
		return _state;
	}

	// Get the current market traded volume.
	auto traded_vol() const -> double
	{
		return _traded_vol;
	}

	// Get MUTABLE reference to underlying runners.
	auto runners() -> runners_t&
	{
		return _runners;
	}

	// Find runner with specified ID, if not present returns nullptr.
	auto find_runner(uint64_t id) -> runner*
	{
		return get_runner(id);
	}

	// Determine if the market contains the specified runner ID.
	auto contains_runner(uint64_t id) -> bool
	{
		return get_runner(id) != nullptr;
	}

	// Retrieves MUTABLE reference to the i-th runner in the market.
	auto operator[](uint64_t i) -> runner&
	{
		return _runners[i];
	}

	// Get the number of runners in the market.
	auto num_runners() const -> uint64_t
	{
		return _num_runners;
	}

	// Get last timestamp market was updated at.
	auto last_timestamp() -> uint64_t
	{
		return _last_timestamp;
	}

	// Set last timestamp market was updated at.
	void set_last_timestamp(uint64_t timestamp)
	{
		_last_timestamp = timestamp;
	}

	// Set market state.
	void set_state(market_state state)
	{
		if (_state == market_state::CLOSED && state != market_state::CLOSED)
			throw std::runtime_error("Cannot move from closed state to " +
						 std::to_string(static_cast<uint64_t>(state)));

		_state = state;
	}

	// Set the market traded volume.
	void set_traded_vol(double vol)
	{
		_traded_vol = vol;
	}

	// Add a new runner to the market with the specified ID. Even though a
	// market will not have runners added during the time the market is
	// live, we want to retain the ability to be able to dynamically
	// generate market data from a data stream without necessarily having
	// the metadata available.
	//
	// Note that we do NOT check for duplicates, the caller must ensure this
	// is a new runner.
	auto add_runner(uint64_t id) -> runner&
	{
		if (_num_runners == MAX_RUNNERS)
			throw std::runtime_error("Already have " + std::to_string(MAX_RUNNERS) +
						 " runners, cannot add another");

		_runner_ids[_num_runners++] = id;
		return _runners.emplace_back(id);
	}

	// Clear mutable market state, retaining immutable state.
	void clear_state()
	{
		_traded_vol = 0;

		for (uint64_t i = 0; i < _num_runners; i++) {
			_runners[i].clear_state();
		}
	}

private:
	uint64_t _id;
	market_state _state;
	double _traded_vol;
	uint64_t _num_runners;
	uint64_t _last_timestamp;
	std::array<uint64_t, MAX_RUNNERS> _runner_ids;
	runners_t _runners;

	// Find a runner with the specified ID, if not present then returns
	// nullptr.
	auto get_runner(uint64_t id) -> runner*
	{
		// Iterating through a small array is faster than most
		// alternative lookup schemes such as hashing.
		for (uint64_t i = 0; i < _num_runners; i++) {
			if (_runner_ids[i] == id)
				return &_runners[i];
		}

		return nullptr;
	}
};
} // namespace janus::betfair
