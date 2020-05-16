#pragma once

#include "bet.hh"
#include "dynamic_array.hh"
#include "market.hh"

#include <cstdint>
#include <vector>

namespace janus
{
static constexpr uint64_t MAX_SIM_BETS = 10000;
// Represents the target volume we specify if the bet is at an invalid level.
static constexpr double INVALID_PRICE_TARGET_VOL = 1e20;

class sim
{
public:
	using bets_t = dynamic_array<bet, MAX_SIM_BETS>;

	explicit sim(betfair::price_range& range, betfair::market& market)
		: _range{range}, _market{market}, _next_bet_id{0}
	{
		init();
	}

	// Add new bet to the simulation.
	//   runner_id: ID of runner bet is made against.
	//       price: Bet price (double since reduction factor can scale to invalid).
	//       stake: Bet stake.
	//     is_back: True for back bets, false for lay.
	//     returns: nullptr if unable to add bet, or a pointer to the added bet otherwise.
	auto add_bet(uint64_t runner_id, double price, double stake, bool is_back) -> bet*;

	// Retrieve simulated bets.
	auto bets() -> bets_t&
	{
		return _bets;
	}

	// Update simulated bets based on current market state.
	void update();

	// Calculate P/L. Should be called once winner determined otherwise
	// returns 0.
	auto pl() -> double;

	// Add a bet to hedge at the specified price (or if not specified best
	// available price) for the specified runner ID (or if not specified all
	// runners).
	auto hedge(uint64_t runner_id = 0, double price = -1) -> bool;

private:
	betfair::price_range& _range;
	betfair::market& _market;
	bets_t _bets;
	dynamic_array<bool, betfair::MAX_RUNNERS> _removed;
	uint64_t _next_bet_id;

	// Implementation of add_bet() with option to bypass checks.
	auto add_bet(uint64_t runner_id, double price, double stake, bool is_back, bool bypass)
		-> bet*;

	// Perform initialisation on object creation.
	void init();

	// Determine the matched volume for the bet's runner at the bet
	// price. If the market has moved such that this price has now swapped
	// side the function returns -1. If better price available, sets bet to
	// that price.
	auto get_matched(bet& bet, betfair::runner& runner) -> double;

	// Determine the target matched volume for the bet's runner at the bet
	// price. If the market has moved such that this price has now swapped
	// side the function returns -1. If better price available, sets bet to
	// that price.
	auto get_target_matched(bet& bet, betfair::runner& runner) -> double;

	// Look up specific runner in the attached market.
	auto find_runner(uint64_t id) -> betfair::runner&;

	// Determine if the runner associated with the bet won.
	auto runner_won(bet& bet) -> bool;

	// Update an individual bet.
	void update_bet(bet& bet);

	// Apply a removal to all bets.
	void apply_removal(double adj_factor);

	// Checks through each of the runners for removals and applies the
	// adjustment factor where appropriate.
	void apply_removals();

	// Determine the volume-weighted average prices on back and lay
	// side. Returns false if no prices.
	auto get_vwap_back_lay(uint64_t runner_id, double& vwap_back, double& vwap_lay,
			       double& vol_back, double& vol_lay) -> bool;

	// Apply hedge to all ACTIVE runners at the specified price (can take
	// best price if set to -1). Returns true if hedge succeeded for each
	// runner.
	auto hedge_all(double price) -> bool;
};
} // namespace janus
