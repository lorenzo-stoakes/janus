#pragma once

#include "ladder.hh"

#include <cstdint>
#include <stdexcept>
#include <string>

namespace janus::betfair
{
enum class runner_state : uint64_t
{
	ACTIVE,
	REMOVED,
	WON,
};

class runner
{
public:
	explicit runner(uint64_t id)
		: _id{id},
		  _state{runner_state::ACTIVE},
		  _traded_vol{0},
		  _adj_factor{0},
		  _ltp_price_index{0}
	{
	}

	// Get runner ID.
	auto id() const -> uint64_t
	{
		return _id;
	}

	// Get runner state.
	auto state() const -> runner_state
	{
		return _state;
	}

	// Indicate that the event is over and the runner won.
	void set_won()
	{
		if (_state != runner_state::ACTIVE)
			throw std::runtime_error("Trying to set runner won when state = " +
						 std::to_string(static_cast<uint64_t>(_state)) +
						 " expected ACTIVE");

		_state = runner_state::WON;
	}

	// Indicate that the runner is removed and set adjustment factor.
	void set_removed(double adj_factor)
	{
		if (_state != runner_state::ACTIVE)
			throw std::runtime_error("Trying to set runner removed when state = " +
						 std::to_string(static_cast<uint64_t>(_state)) +
						 " expected ACTIVE");

		_state = runner_state::REMOVED;
		_adj_factor = adj_factor;
	}

	// Get runner traded volume.
	auto traded_vol() const -> double
	{
		return _traded_vol;
	}

	// Set runner traded volume.
	void set_traded_vol(double vol)
	{
		_traded_vol = vol;
	}

	// Get runner adjustment factor. Will only be non-zero if the state is
	// REMOVED.
	auto adj_factor() const -> double
	{
		return _adj_factor;
	}

	// Get price index of runner Last Traded Price (LTP).
	auto ltp() const -> uint64_t
	{
		return _ltp_price_index;
	}

	// Set price index of LTP for runner.
	void set_ltp(uint64_t price_index)
	{
		_ltp_price_index = price_index;
	}

	// Get runner (mutable) underlying ladder.
	auto ladder() -> ladder&
	{
		return _ladder;
	}

private:
	uint64_t _id;
	runner_state _state;
	double _traded_vol;
	double _adj_factor;
	uint64_t _ltp_price_index;

	janus::betfair::ladder _ladder;
};
} // namespace janus::betfair
