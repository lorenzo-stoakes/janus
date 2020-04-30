#pragma once

#include "janus.hh"

#include <cstdint>
#include <sstream>
#include <stdexcept>
#include <string>

namespace janus::betfair
{
template<uint64_t Cap>
void universe<Cap>::apply_market_id(uint64_t id)
{
	_last_market = find_market(id);
	if (_last_market == nullptr)
		_last_market = &add_market(id);

	// Changing market invalidates the last known runner.
	_last_runner = nullptr;
}

template<uint64_t Cap>
void universe<Cap>::apply_runner_id(uint64_t id)
{
	_last_runner = _last_market->find_runner(id);
	if (_last_runner == nullptr)
		_last_runner = &_last_market->add_runner(id);
}

template<uint64_t Cap>
void universe<Cap>::apply_timestamp(uint64_t timestamp)
{
	// The ISO-8601 timestamp buffer is 25 characters long.
	static constexpr uint64_t MAX_TIMESTAMP_SIZE = 25;

	if (_last_timestamp != 0 && timestamp < _last_timestamp) {
		std::array<char, MAX_TIMESTAMP_SIZE> buf{};
		std::ostringstream oss;
		oss << "ERROR: Timestamp goes backwards?! ";
		oss << "prev timestamp=" << _last_timestamp << " ("
		    << print_iso8601(&buf[0], _last_timestamp) << ") ";
		oss << "timestamp=" << timestamp << " (" << print_iso8601(&buf[0], timestamp)
		    << ") ";

		// Reset last timestamp so we don't unnecessarily repeat errors.
		_last_timestamp = timestamp;
		throw std::runtime_error(oss.str());
	}

	_last_timestamp = timestamp;
}

template<uint64_t Cap>
void universe<Cap>::set_market_timestamp()
{
	_last_market->set_last_timestamp(_last_timestamp);
}

template<uint64_t Cap>
void universe<Cap>::set_runner_timestamp()
{
	_last_runner->set_last_timestamp(_last_timestamp);
}

template<uint64_t Cap>
void universe<Cap>::apply_market_clear()
{
	_last_market->clear_state();
	// Clearing the market invalidates the last known runner.
	_last_runner = nullptr;
}

template<uint64_t Cap>
void universe<Cap>::apply_market_state(market_state state)
{
	_last_market->set_state(state);
}

template<uint64_t Cap>
void universe<Cap>::apply_market_inplay()
{
	_last_market->set_inplay(true);
}

template<uint64_t Cap>
void universe<Cap>::apply_market_traded_vol(double vol)
{
	_last_market->set_traded_vol(vol);
}

template<uint64_t Cap>
void universe<Cap>::apply_runner_removal(double adj_factor)
{
	// Further updates will scale matched/LTP accordingly.
	_last_runner->set_removed(adj_factor);
}

template<uint64_t Cap>
void universe<Cap>::apply_runner_clear_unmatched()
{
	_last_runner->ladder().clear_unmatched();
}

template<uint64_t Cap>
void universe<Cap>::apply_runner_traded_vol(double vol)
{
	_last_runner->set_traded_vol(vol);
}

template<uint64_t Cap>
void universe<Cap>::apply_runner_ltp(uint64_t price_index)
{
	_last_runner->set_ltp(price_index);
}

template<uint64_t Cap>
void universe<Cap>::apply_runner_matched(uint64_t price_index, double vol)
{
	ladder& ladder = _last_runner->ladder();
	ladder.set_matched_at(price_index, vol);
}

template<uint64_t Cap>
void universe<Cap>::apply_runner_unmatched(uint64_t price_index, double vol)
{
	ladder& ladder = _last_runner->ladder();
	ladder.set_unmatched_at(price_index, vol);
}

template<uint64_t Cap>
void universe<Cap>::apply_runner_sp(double sp)
{
	_last_runner->set_sp(sp);
}
template<uint64_t Cap>
void universe<Cap>::apply_runner_won()
{
	_last_runner->set_won();
}

template<uint64_t Cap>
void universe<Cap>::do_apply_update(const update& update)
{
	update_type type = update.type;

	// Handle market ID.

	// If we haven't set any market ID yet we can't do anything else.
	if (_last_market == nullptr && type != update_type::MARKET_ID &&
	    type != update_type::TIMESTAMP)
		throw std::runtime_error(std::string("Received ") + update_type_str(type) +
					 " update but no market ID received yet?!");

	if (type == update_type::MARKET_ID) {
		apply_market_id(get_update_market_id(update));
		_num_updates++;
		return;
	}

	// Handle runner ID.

	bool was_runner_update = is_runner_update(type);
	// If we haven't set any runner ID yet then we can't apply runner updates.
	if (_last_runner == nullptr && was_runner_update)
		throw std::runtime_error(
			std::string("Received ") + update_type_str(type) + " update for market " +
			std::to_string(_last_market->id()) + " but no runner ID received yet?!");

	if (type == update_type::RUNNER_ID) {
		apply_runner_id(get_update_runner_id(update));
		_num_updates++;
		return;
	}

	// Handle timestamp.

	if (type == update_type::TIMESTAMP) {
		apply_timestamp(get_update_timestamp(update));
		_num_updates++;
		return;
	}

	// If we haven't seen a timestamp yet we can't apply any non-market ID/runner ID updates.
	if (_last_timestamp == 0)
		throw std::runtime_error(std::string("Received ") + update_type_str(type) +
					 " update but no timestamp received yet?!");

	// Handle all other updates.

	// This is a market/runner update so market timestamp should be set.
	set_market_timestamp();
	// If it's a runner update we set runner timestamp too.
	if (was_runner_update)
		set_runner_timestamp();

	switch (type) {
	case update_type::MARKET_CLEAR:
		apply_market_clear();
		break;
	case update_type::MARKET_OPEN:
		apply_market_state(market_state::OPEN);
		break;
	case update_type::MARKET_CLOSE:
		apply_market_state(market_state::CLOSED);
		break;
	case update_type::MARKET_SUSPEND:
		apply_market_state(market_state::SUSPENDED);
		break;
	case update_type::MARKET_INPLAY:
		apply_market_inplay();
		break;
	case update_type::MARKET_TRADED_VOL:
		apply_market_traded_vol(get_update_market_traded_vol(update));
		break;
	case update_type::RUNNER_REMOVAL:
		apply_runner_removal(get_update_runner_adj_factor(update));
		break;
	case update_type::RUNNER_CLEAR_UNMATCHED:
		apply_runner_clear_unmatched();
		break;
	case update_type::RUNNER_TRADED_VOL:
		apply_runner_traded_vol(get_update_runner_traded_vol(update));
		break;
	case update_type::RUNNER_LTP:
		apply_runner_ltp(get_update_runner_ltp(update));
		break;
	case update_type::RUNNER_MATCHED:
	{
		auto [price_index, vol] = get_update_runner_matched(update);
		apply_runner_matched(price_index, vol);
		break;
	}
	case update_type::RUNNER_UNMATCHED_ATL:
	{
		auto [price_index, vol] = get_update_runner_unmatched_atl(update);
		// ATL so positive volume.
		apply_runner_unmatched(price_index, vol);

		break;
	}
	case update_type::RUNNER_UNMATCHED_ATB:
	{
		auto [price_index, vol] = get_update_runner_unmatched_atb(update);
		// ATB so negative volume.
		apply_runner_unmatched(price_index, -vol);
		break;
	}
	case update_type::RUNNER_SP:
		apply_runner_sp(get_update_runner_sp(update));
		break;
	case update_type::RUNNER_WON:
		apply_runner_won();
		break;
	default:
		throw std::runtime_error(std::string("Dind't catch update type ") +
					 update_type_str(type));
	}

	_num_updates++;
}

template<uint64_t Cap>
void universe<Cap>::apply_update(const update& update)
{
	try {
		do_apply_update(update);
	} catch (std::exception& e) {
		uint64_t market_id = _last_market == nullptr ? 0 : _last_market->id();
		uint64_t runner_id = _last_runner == nullptr ? 0 : _last_runner->id();
		throw universe_apply_error(market_id, runner_id, e);
	}
}
} // namespace janus::betfair
