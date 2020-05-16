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

	// Note that this will set the correct best available price if the input
	// price is suboptimal.
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

	double ret = 0;
	for (uint64_t i = 0; i < _bets.size(); i++) {
		bet& bet = _bets[i];
		double delta = bet.pl(runner_won(bet));

		ret += delta;
	}

	return ret;
}

/*
 * Calculate hedge side, volume at specified price level against input back/lay
 * price/volume pairs.
 *
 * Our aim is to create a bet which results in the same profit
 * regardless of outcome (we ignore commission rates for the purposes of
 * this calculation).
 *
 * If we are hedging on the lay side:
 *
 * P/L if back is:
 * (£_back - 1) * vol_back - (£_lay - 1) * vol_lay - (£_hedge - 1) * vol_hedge
 *
 * P/L if lay is:
 * vol_lay + vol_hedge - vol_back
 *
 * ∴ (£_back - 1) * vol_back - (£_lay - 1) * vol_lay - (£_hedge - 1) * vol_hedge =
 *     vol_lay + vol_hedge - vol_back
 * ∴ £_back * vol_back - £_lay * vol_lay - £_hedge * vol_hedge = 0
 * ∴ vol_hedge = (£_back * vol_back - £_lay * vol_lay) / £_hedge
 *
 * If we are hedging on the back side:
 *
 * P/L if back is:
 * (£_back - 1) * vol_back - (£_lay - 1) * vol_lay + (£_hedge - 1) * vol_hedge
 *
 * P/L if lay is:
 * vol_lay - vol_hedge - vol_back
 *
 * ∴ (£_back - 1) * vol_back - (£_lay - 1) * vol_lay + (£_hedge - 1) * vol_hedge =
 *    vol_lay - vol_hedge - vol_back
 * ∴ £_back * vol_back - £_lay * vol_lay + £_hedge * vol_hedge = 0
 * ∴ vol_hedge = (£_lay * vol_lay - £_back * vol_back) / £_hedge
 *
 * ∴ vol_hedge = |(£_lay * vol_lay - £_back * vol_back) / £_hedge|
 * ∴   is_back = vol_lay >= vol_back (when equal we could choose either, back by convention).
 *
 * Note that £_hedge is the parameter 'price' - the specified hedge price.
 */
static auto calc_hedge(double price_back, double vol_back, double price_lay, double vol_lay,
		       double price) -> std::pair<bool, double>
{
	// We assume the caller confirms price is valid value.

	bool is_back = vol_lay >= vol_back;
	double stake = ::fabs(price_lay * vol_lay - price_back * vol_back) / price;
	return std::make_pair(is_back, stake);
}

auto sim::get_vwap_back_lay(uint64_t runner_id, double& vwap_back, double& vwap_lay,
			    double& vol_back, double& vol_lay) -> bool
{
	// We calculate the average price weighted by volume for both back and
	// lay. This is useful because you can treat an entire set of back/lay
	// bets on a runner as equivalent to a single pair of VWAPs on the back
	// and lay side.

	vol_back = 0;
	vol_lay = 0;
	double sum_back = 0, sum_lay = 0;
	for (uint64_t i = 0; i < _bets.size(); i++) {
		bet& bet = _bets[i];

		if (bet.runner_id() != runner_id)
			continue;

		double matched = bet.matched();

		if (bet.is_back()) {
			sum_back += bet.price() * matched;
			vol_back += matched;
		} else {
			sum_lay += bet.price() * matched;
			vol_lay += matched;
		}
	}

	vwap_back = vol_back > 0 ? sum_back / vol_back : 0;
	vwap_lay = vol_lay > 0 ? sum_lay / vol_lay : 0;

	return !dz(vwap_back) || !dz(vwap_lay);
}

auto sim::hedge_all(double price) -> bool
{
	bool ret = true;
	betfair::market::runners_t& runners = _market.runners();
	for (uint64_t i = 0; i < runners.size(); i++) {
		betfair::runner& runner = runners[i];
		if (runner.state() != betfair::runner_state::ACTIVE)
			continue;

		// Avoid infinite loop. Shouldn't be possible.
		uint64_t id = runner.id();
		if (id == 0)
			throw std::runtime_error("Runner with 0 ID??");

		if (!hedge(id, price))
			ret = false;
	}

	return ret;
}

auto sim::hedge(uint64_t runner_id, double price) -> bool
{
	if (runner_id == 0)
		return hedge_all(price);

	// Firstly obtain the Volume-Weighted Average Price for MATCHED back and
	// lay for this runner - this pair is equivalent to all matched back and
	// lay bets on the runner.

	double vwap_back, vwap_lay;
	double vol_back, vol_lay;
	if (!get_vwap_back_lay(runner_id, vwap_back, vwap_lay, vol_back, vol_lay))
		return false;

	if (price < 0) {
		// Negative price indicates we should hedge at best price we can.
		bool is_hedge_back = vol_lay >= vol_back;
		// The price will get scaled accordingly.
		if (is_hedge_back)
			price = 1.01;
		else
			price = 1000.;
	} else if (price < 1.01 || price > 1000) { // Ignore invalid prices.
		return false;
	}

	auto [is_back, stake] = calc_hedge(vwap_back, vol_back, vwap_lay, vol_lay, price);

	if (dz(stake))
		return false;

	auto* bet = add_bet(runner_id, price, stake, is_back, true);
	if (bet == nullptr)
		return false;

	// Scale bet stake/matched such that return remains the same in the
	// case where orig_price and price differ (i.e. a better price was
	// obtained.)
	// Payout 1 = orig_price * orig_vol
	// Payout 2 = new_price * vol_new
	// ∴ orig_price * orig_vol = new_price * vol_new
	// ∴ vol_new = orig_vol * orig_price / new_price
	double mult = price / bet->price();

	if (!deq(mult, 1)) {
		bet->scale_stake_sim(mult);
		// If we have scaled we have to update the sim to take into account that
		// the scaled portion could now include some unmatched component.
		update_bet(*bet);
	}

	return true;
}
} // namespace janus
