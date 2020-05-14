#include "janus.hh"

#include <cmath>
#include <sstream>
#include <stdexcept>
#include <string>

namespace janus
{
void sim::init()
{
	// Initialise removal state for each runner.
	betfair::market::runners_t& runners = _market.runners();
	for (uint64_t i = 0; i < runners.size(); i++) {
		betfair::runner& runner = runners[i];
		_removed.emplace_back(runner.state() == betfair::runner_state::REMOVED);
	}
}

auto sim::get_matched(bet& bet, betfair::runner& runner) -> double
{
	// Note that this expects the caller to have determined that this is a
	// valid runner and bet.

	// TODO(lorenzo): perf: Good idea to do this every time? Maybe store
	// price index in bet as well as double value (for adjustment
	// factor-invoked invalid prices).
	uint64_t price_index = _range.price_to_nearest_index(bet.price());

	// If we cannot determine price, don't raise an error, rather indicate
	// that no traded volume is present. This is to reduce simulation run
	// fragility.
	if (price_index == betfair::INVALID_PRICE_INDEX)
		return 0;

	betfair::ladder& ladder = runner.ladder();
	bool is_back = bet.is_back();

	// Indicate that the level we are targeting has already been subsumed
	// by the other side of the bet.
	if (is_back) {
		uint64_t max_atb = ladder.max_atb_index();
		if (max_atb >= price_index) {
			// Provide best price.
			bet.set_price(betfair::price_range::index_to_price(max_atb));

			return -1;
		}
	} else {
		uint64_t min_atl = ladder.min_atl_index();
		if (min_atl <= price_index) {
			// Provide best price.
			bet.set_price(betfair::price_range::index_to_price(min_atl));

			return -1;
		}
	}

	return ladder.matched(price_index);
}

auto sim::get_target_matched(bet& bet, betfair::runner& runner) -> double
{
	double matched = get_matched(bet, runner);
	if (matched == -1)
		return -1;

	// TODO(lorenzo): perf: We duplicate this with get_matched(). Since it
	// only happens on initial bet placement it is probably not a huge big
	// deal.
	uint64_t price_index = _range.price_to_nearest_index(bet.price());
	if (price_index == betfair::INVALID_PRICE_INDEX)
		return INVALID_PRICE_TARGET_VOL;

	betfair::ladder& ladder = runner.ladder();
	// We take a pessimistic view - we assume the existing unmatched queue
	// will not have any bets removed and we are at its end.
	double queue_length = ::fabs(ladder.unmatched(price_index));

	// Matched volume is expressed in terms of both back and lay, so we have
	// to multiply by 2 to get the correct target matched volume.
	return matched + 2 * queue_length;
}

auto sim::add_bet(uint64_t runner_id, double price, double stake, bool is_back) -> bet*
{
	return add_bet(runner_id, price, stake, is_back, false);
}

auto sim::add_bet(uint64_t runner_id, double price, double stake, bool is_back, bool bypass) -> bet*
{
	// If the bet is patently ridiculous don't allow it.
	if (stake < 0 || price < 1)
		return nullptr;

	// If the market is suspended or closed then clearly we cannot place a
	// bet.
	if (!bypass && _market.state() != betfair::market_state::OPEN)
		return nullptr;

	// Currently we do not support simulating inplay bets.
	if (!bypass && _market.inplay())
		return nullptr;

	betfair::runner& runner = find_runner(runner_id);
	// Can only bet on active runners.
	if (!bypass && runner.state() != betfair::runner_state::ACTIVE)
		return nullptr;

	bet& bet = _bets.emplace_back(runner_id, price, stake, is_back, true);
	bet.set_bet_id(_next_bet_id++);

	double target_matched = get_target_matched(bet, runner);
	if (target_matched < 0) {
		// When the market has moved past our bet this simulation
		// simplifies things by simply matching the whole bet.
		// TODO(lorenzo): Reconsider.
		bet.match(stake);
	} else {
		// Otherwise, we set a _target matched_ value for the bet. This
		// determines the matched volume at the bet's price level at
		// which we can start matching this bet.
		bet.set_target_matched(target_matched);
	}

	return &bet;
}

auto sim::find_runner(uint64_t id) -> betfair::runner&
{
	// TODO(lorenzo): perf: Looking up runner per-bet?
	betfair::runner* runner = _market.find_runner(id);
	if (runner == nullptr) {
		std::ostringstream oss;
		oss << "Cannot find runner " << id << " in market " << _market.id();
		throw std::runtime_error(oss.str());
	}

	return *runner;
}

void sim::update_bet(bet& bet)
{
	betfair::runner& runner = find_runner(bet.runner_id());
	if (runner.state() == betfair::runner_state::REMOVED) {
		bet.void_bet();
		return;
	}

	// Other than voided bets (which could have been completed before being
	// voided) handled above we are not interested in any other completed
	// bets.
	if (bet.is_complete())
		return;

	double matched = get_matched(bet, runner);
	// If the market has moved over us, we simply mark the bet fully
	// matched. TODO(lorenzo): Reconsider.
	if (matched == -1) {
		bet.match(bet.unmatched());
		return;
	}

	double target = bet.target_matched();
	if (matched > target) {
		double diff = matched - target;
		// Since the matched volume contains both back and lay side we
		// have to divide by 2 to determine what portion of our bet has
		// matched.
		diff /= 2.;

		// This handles the case where diff > unmatched.
		bet.match(diff);
	}
}

void sim::apply_removal(double adj_factor)
{
	uint64_t size = _bets.size();
	for (uint64_t i = 0; i < size; i++) {
		bet& bet = _bets[i];
		// Scale matched bets, split out unmatched portion of back bets
		// according to betfair rules.
		double split_stake = 0;
		if (bet.apply_adj_factor(adj_factor, split_stake))
			add_bet(bet.runner_id(), bet.orig_price(), split_stake, bet.is_back(),
				true);
	}
}

void sim::apply_removals()
{
	// Note that bets on the removed horses are voided in update_bet().
	betfair::market::runners_t& runners = _market.runners();

	for (uint64_t i = 0; i < runners.size(); i++) {
		// If we already know about this removal then nothing to do.
		if (_removed[i])
			continue;

		betfair::runner& runner = runners[i];
		if (runner.state() != betfair::runner_state::REMOVED)
			continue;

		apply_removal(runner.adj_factor());
		_removed[i] = true;
	}
}

void sim::update()
{
	if (_market.state() != betfair::market_state::OPEN)
		return;

	for (uint64_t i = 0; i < _bets.size(); i++) {
		bet& bet = _bets[i];
		update_bet(bet);
	}

	// TODO(lorenzo): perf: Check this on each update?
	apply_removals();
}

auto sim::runner_won(bet& bet) -> bool
{
	betfair::runner& runner = find_runner(bet.runner_id());
	return runner.state() == betfair::runner_state::WON;
}

auto sim::pl() -> double
{
	double ret = 0;

	// Ensure at least one of the runners won otherwise we return 0.
	bool saw_won = false;
	betfair::market::runners_t& runners = _market.runners();
	for (uint64_t i = 0; i < runners.size(); i++) {
		betfair::runner& runner = runners[i];
		if (runner.state() == betfair::runner_state::WON) {
			saw_won = true;
			break;
		}
	}
	if (!saw_won)
		return 0;

	for (uint64_t i = 0; i < _bets.size(); i++) {
		bet& bet = _bets[i];
		bool won = runner_won(bet);
		ret += bet.pl(won);
	}

	return ret;
}
} // namespace janus
