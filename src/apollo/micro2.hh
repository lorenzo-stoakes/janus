#pragma once

#include "janus.hh"

#include <array>
#include <cmath>
#include <iostream>
#include <vector>

//#define PRINT_OUT

namespace janus::apollo::micro2
{
static constexpr uint64_t STAKE_SIZE = 500;
static constexpr uint64_t MAX_HIST_PRICES = 500;
static constexpr uint64_t MAX_RUNNERS = 50;

static constexpr double MIN_MARKET_TRADED_VOL = 50000;

static constexpr uint64_t HISTORY_DURATION_MS = 30 * 1000;

static constexpr uint64_t MAX_PRICEX100 = 680;

static constexpr double MIN_FAV_ATL_VOL = 10;

static constexpr uint64_t EXIT_DELAY_MS = 30 * 1000;
// static constexpr uint64_t EXIT_DELAY_MS = 0;

static constexpr uint64_t MIN_CLOSEST_TICKS = 10;

static constexpr double MIN_TREND_DELTA = 1.2;
static constexpr double MAX_TREND_DELTA = 10;

static constexpr double MIN_RUNNER_TRADED_VOL = 5000;

static constexpr uint64_t DELTA_TICKS_BET = 3;

class strat
{
public:
	strat()
		: _analyser(predicate, update_worker, market_reducer, node_reducer, reducer,
			    zero_worker_state, zero_market_agg_state, zero_node_agg_state)
	{
	}

	void run()
	{
		const janus::config& config = janus::parse_config();

#ifdef PRINT_OUT
		auto res = _analyser.run(config, 1);
#else
		auto res = _analyser.run(config);
#endif
		std::cout << res.pl << std::endl;
	}

private:
	struct runner_state
	{
		std::array<uint64_t, MAX_HIST_PRICES> prev_prices;
		std::array<uint64_t, MAX_HIST_PRICES> timestamps;
		uint64_t num_prices;
		uint64_t offset;

		auto last_price() -> uint64_t
		{
			if (num_prices == 0)
				return 0;

			if (offset == 0)
				return prev_prices[num_prices - 1];

			return prev_prices[offset - 1];
		}

		void add_price(uint64_t timestamp, uint64_t price_index)
		{
			uint64_t last_index = last_price();
			uint64_t diff = price_index < last_index ? last_index - price_index
								 : price_index - last_index;
			if (diff < 1)
				return;

			if (num_prices < MAX_HIST_PRICES) {
				prev_prices[num_prices] = price_index;
				timestamps[num_prices] = timestamp;
				num_prices++;
				return;
			}

			prev_prices[offset] = price_index;
			timestamps[offset] = timestamp;
			offset = (offset + 1) % num_prices;
		}

		auto trend(uint64_t timestamp, uint64_t duration) -> double
		{
			uint64_t min_timestamp = timestamp - duration;

			uint64_t ind = offset;
			uint64_t sum = 0;
			uint64_t count = 0;
			for (uint64_t i = 0; i < num_prices; i++) {
				uint64_t timestamp = timestamps[ind];
				if (timestamp >= min_timestamp) {
					uint64_t price_index = prev_prices[ind];
					sum += price_index;
					count++;
				}

				ind = (ind + 1) % num_prices;
			}

			if (count == 0)
				return 0;

			double av_prev = static_cast<double>(sum) / static_cast<double>(count);
			double last_seen = static_cast<double>(last_price());
			return last_seen - av_prev;
		}
	};

	struct worker_state
	{
		std::array<runner_state, MAX_RUNNERS> prev_states;
		bool entered;
		bool exited;
		uint64_t enter_timestamp;
	};

	struct market_agg_state
	{
		double pl;
	};

	struct node_agg_state
	{
		double pl;
	};

	struct result
	{
		double pl;
	};

	const worker_state zero_worker_state = {
		.prev_states = {0},
		.entered = false,
		.exited = false,
		.enter_timestamp = 0,
	};

	const market_agg_state zero_market_agg_state = {
		.pl = 0,
	};

	const node_agg_state zero_node_agg_state = {
		.pl = 0,
	};

	analyser<worker_state, market_agg_state, node_agg_state, result> _analyser;

	static auto predicate(const janus::meta_view& meta, const janus::stats& stats) -> bool
	{
		// Lingfield 6f Mdn Stakes 2020-06-05 13:30
		// return market_id == 170630572;
#ifdef PRINT_OUT
		uint64_t market_id = meta.market_id();

		if (market_id == 168799745 || market_id == 158451594 || market_id == 163419128 ||
		    market_id == 160150729 || market_id == 158985763) {
			std::cout << market_id << ": " << meta.describe(true) << std::endl;
			return true;
		}

		return false;
#endif
		// Leave rest of logic for informational purposes.

		// We only want horse races.
		if (meta.event_type_id() != 7)
			return false;

		// We have to have a full race.
		auto flags = stats.flags;
		if ((flags & janus::stats_flags::WENT_INPLAY) != janus::stats_flags::WENT_INPLAY ||
		    (flags & janus::stats_flags::WAS_CLOSED) != janus::stats_flags::WAS_CLOSED ||
		    (flags & janus::stats_flags::SAW_WINNER) != janus::stats_flags::SAW_WINNER)
			return false;

		// We want to see a decent update interval pre-off.
		auto& five_mins = stats.pre_post_intervals[static_cast<int>(
			janus::stats_interval::FIVE_MINS)];
		auto& three_mins = stats.pre_post_intervals[static_cast<int>(
			janus::stats_interval::THREE_MINS)];
		auto& one_min =
			stats.pre_post_intervals[static_cast<int>(janus::stats_interval::ONE_MIN)];
		if (five_mins.mean_update_interval_ms > 200 ||
		    five_mins.worst_update_interval_ms > 2000 ||
		    three_mins.mean_update_interval_ms > 150 ||
		    three_mins.worst_update_interval_ms > 2000 ||
		    one_min.mean_update_interval_ms > 100 ||
		    one_min.worst_update_interval_ms > 1000)
			return false;

		return true;
	}

	static auto update_worker(int core, const janus::meta_view& meta,
				  janus::betfair::market& market, janus::sim& sim,
				  const node_agg_state& node_agg_state, worker_state& state,
				  spdlog::logger* logger) -> bool
	{
		uint64_t market_timestamp = market.last_timestamp();

		if (market.state() != betfair::market_state::OPEN)
			return true;

		// Don't look after post.
		uint64_t start_timestamp = meta.market_start_timestamp();
		if (market_timestamp > start_timestamp) {
			sim.cancel_all();
			sim.hedge();
			return true;
		}

		if (state.entered) {
			if (state.exited)
				return true;

			uint64_t delta = market_timestamp - state.enter_timestamp;

			if (EXIT_DELAY_MS > 0 && delta >= EXIT_DELAY_MS) {
				sim.cancel_all();
				sim.hedge();
				state.exited = true;
			}

			return true;
		}

		uint64_t fav_id = 0;
		uint64_t fav_id_2nd = 0;
		uint64_t fav_index = 0;
		uint64_t fav_index_2nd = 0;
		uint64_t fav_price_index = betfair::NUM_PRICES;
		uint64_t num_favs = 0;

		double fav_atl_vol = 0;
		double fav_atl_vol_2nd = 0;

		bool invalid_fav = false;

		for (uint64_t i = 0; i < market.runners().size(); i++) {
			auto& runner = market[i];

			if (runner.state() != betfair::runner_state::ACTIVE)
				continue;

			uint64_t price_index = runner.ladder().best_atl().first;
			uint64_t best_atb = runner.ladder().best_atb().first;

			state.prev_states[i].add_price(market_timestamp, price_index);

			bool invalid = price_index - best_atb > 1 || price_index == 0 ||
				       price_index == betfair::NUM_PRICES - 1 ||
				       runner.traded_vol() < MIN_RUNNER_TRADED_VOL;

			if (price_index < fav_price_index) {
				num_favs = 1;
				fav_price_index = price_index;
				fav_index = i;
				fav_id = runner.id();
				fav_atl_vol = runner.ladder().unmatched(price_index);

				invalid_fav = invalid;
			} else if (price_index == fav_price_index) {
				fav_index_2nd = i;
				fav_id_2nd = runner.id();
				fav_atl_vol_2nd = runner.ladder().unmatched(price_index);
				num_favs++;

				if (invalid_fav || invalid)
					invalid_fav = true;
			}
		}

		if (invalid_fav)
			return true;

		if (market.traded_vol() < MIN_MARKET_TRADED_VOL)
			return true;

		if (num_favs < 2)
			return true;

		if (betfair::price_range::index_to_pricex100(fav_price_index) > MAX_PRICEX100)
			return true;

		if (fav_atl_vol < MIN_FAV_ATL_VOL || fav_atl_vol_2nd < MIN_FAV_ATL_VOL)
			return true;

		uint64_t next_price_index = betfair::NUM_PRICES;
		for (uint64_t i = 0; i < market.runners().size(); i++) {
			auto& runner = market[i];
			if (runner.state() != betfair::runner_state::ACTIVE)
				continue;

			if (i == fav_index || i == fav_index_2nd)
				continue;

			uint64_t price_index = runner.ladder().best_atl().first;
			if (price_index < next_price_index)
				next_price_index = price_index;
		}

		if (next_price_index - fav_price_index < MIN_CLOSEST_TICKS)
			return true;

		double trend1 =
			state.prev_states[fav_index].trend(market_timestamp, HISTORY_DURATION_MS);
		double trend2 = state.prev_states[fav_index_2nd].trend(market_timestamp,
								       HISTORY_DURATION_MS);
		if (trend1 == 0 || trend2 == 0)
			return true;

		bool drift1 = trend1 > 0;
		bool drift2 = trend2 > 0;

		// Need 1 to drift and the other to steam.
		if ((drift1 && drift2) || (!drift1 && !drift2))
			return true;

		if (::fabs(trend1) < MIN_TREND_DELTA || ::fabs(trend2) < MIN_TREND_DELTA)
			return true;

		if (::fabs(trend1) > MAX_TREND_DELTA || ::fabs(trend2) > MAX_TREND_DELTA)
			return true;

		if (drift1) {
			// Lay 1, back 2.
			// sim.add_bet(fav_id, 1000, STAKE_SIZE, false);
			// sim.add_bet(fav_id_2nd, 1.01, STAKE_SIZE, true);

			sim.add_bet(fav_id, fav_price_index - DELTA_TICKS_BET - 1, STAKE_SIZE,
				    false);
			sim.add_bet(fav_id_2nd, fav_price_index + DELTA_TICKS_BET, STAKE_SIZE,
				    true);
		} else {
			// Back 1, lay 2.
			// sim.add_bet(fav_id, 1.01, STAKE_SIZE, true);
			// sim.add_bet(fav_id_2nd, 1000, STAKE_SIZE, false);

			sim.add_bet(fav_id, fav_price_index + DELTA_TICKS_BET, STAKE_SIZE, true);
			sim.add_bet(fav_id_2nd, fav_price_index - DELTA_TICKS_BET - 1, STAKE_SIZE,
				    false);
		}

#ifdef PRINT_OUT
		double price = betfair::price_range::index_to_price(fav_price_index);
		logger->info(
			"Core {}: market {}: timestamp={}, ID 1={}, ID 2={}, price={}, trend 1={}, trend 2={}, market traded vol={} ",
			core, market.id(), market_timestamp, fav_id, fav_id_2nd, price, trend1,
			trend2, market.traded_vol());
#endif

		state.entered = true;
		state.enter_timestamp = market_timestamp;

		return true;
	}

	static auto market_reducer(int core, const worker_state& worker_state, janus::sim& sim,
				   betfair::market& market, bool worker_aborted,
				   market_agg_state& state, spdlog::logger* logger) -> bool
	{
		if (sim.bets().size() > 0) {
			double pl = sim.pl();
			state.pl += pl;
			logger->info("Core {}: Market {}: P/L {}", core, market.id(), pl);
		}

		return true;
	}

	static auto node_reducer(int core, const market_agg_state& market_agg_state,
				 bool market_reducer_aborted, node_agg_state& state,
				 spdlog::logger* logger) -> bool
	{
		state.pl = market_agg_state.pl;
		return false;
	}

	static auto reducer(const std::vector<node_agg_state>& states) -> result
	{
		result ret = {
			.pl = 0,
		};

		for (const auto& state : states) {
			ret.pl += state.pl;
		}

		return ret;
	}
};
} // namespace janus::apollo::micro2
