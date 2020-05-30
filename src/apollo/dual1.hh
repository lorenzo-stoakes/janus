#pragma once

#include "janus.hh"

#include <array>
#include <iostream>
#include <vector>

// #define DUMP_CONFIG_DUAL

namespace janus::apollo
{
struct config_dual
{
	uint64_t pre_post_ms;
	uint64_t num_ticks;
	double min_vol;
	bool reverse;
	bool both;
};

static constexpr double STAKE_SIZE_DOUBLE = 10;

static constexpr uint64_t MAX_RUNNER_PRICEX100_DUAL = 1200;

static constexpr uint64_t NUM_PRE_POST_MS_DUAL = 2;
const static std::array<uint64_t, NUM_PRE_POST_MS_DUAL> pre_post_ms_params_dual = {5 * 60 * 1000,
										   3 * 60 * 1000};

static constexpr uint64_t NUM_TICKS_DUAL = 3;
const static std::array<uint64_t, NUM_TICKS_DUAL> num_ticks_params_dual = {10, 5, 3};

static constexpr uint64_t NUM_MIN_VOLS_DUAL = 2;
const static std::array<double, NUM_MIN_VOLS_DUAL> min_vol_params_dual = {50000, 30000};

static constexpr uint64_t NUM_REVERSE_DUAL = 3;

static constexpr uint64_t TOTAL_NUM_CONFIGS_DUAL =
	NUM_PRE_POST_MS_DUAL * NUM_TICKS_DUAL * NUM_MIN_VOLS_DUAL * NUM_REVERSE_DUAL;

static std::array<config_dual, TOTAL_NUM_CONFIGS_DUAL> configs_dual;

static inline void print_config_dual(uint64_t& index, config_dual& conf)
{
	std::string dir;
	if (conf.both)
		dir = "BOTH";
	else if (conf.reverse)
		dir = "REVERSE";
	else
		dir = "NORMAL";

	std::cout << index << "\t" << conf.pre_post_ms << "\t" << conf.num_ticks << "\t"
		  << conf.min_vol << "\t" << dir << std::endl;
}

static inline void init_configs_dual5(uint64_t& index, config_dual& conf)
{
#ifdef DUMP_CONFIG_DUAL
	print_config_dual(index, conf);
#endif

	configs_dual[index++] = conf;
}

static inline void init_configs_dual4(uint64_t& index, config_dual& conf)
{
	conf.both = false;
	conf.reverse = false;
	init_configs_dual5(index, conf);

	conf.both = false;
	conf.reverse = true;
	init_configs_dual5(index, conf);

	conf.both = true;
	conf.reverse = false;
	init_configs_dual5(index, conf);
}

static inline void init_configs_dual3(uint64_t& index, config_dual& conf)
{
	for (uint64_t i = 0; i < NUM_MIN_VOLS_DUAL; i++) {
		conf.min_vol = min_vol_params_dual[i];
		init_configs_dual4(index, conf);
	}
}

static inline void init_configs_dual2(uint64_t& index, config_dual& conf)
{
	for (uint64_t i = 0; i < NUM_TICKS_DUAL; i++) {
		conf.num_ticks = num_ticks_params_dual[i];
		init_configs_dual3(index, conf);
	}
}

static inline void init_configs_dual()
{
	uint64_t index = 0;

	for (uint64_t i = 0; i < NUM_PRE_POST_MS_DUAL; i++) {
		config_dual conf = {0};
		conf.pre_post_ms = pre_post_ms_params_dual[i];
		init_configs_dual2(index, conf);
	}
}

class dual1
{
public:
	dual1()
		: _analyser(predicate, update_worker, market_reducer, node_reducer, reducer,
			    zero_worker_state, zero_market_agg_state, zero_node_agg_state)
	{
	}

	void run()
	{
		const janus::config& config = janus::parse_config();

		auto res = _analyser.run(config);

		for (uint64_t i = 0; i < TOTAL_NUM_CONFIGS_DUAL; i++) {
			std::cout << res.pls[i] << "\t" << i << "\t" << res.num_enters[i]
				  << std::endl;
		}
	}

private:
	struct worker_state
	{
		bool entered;
		std::array<uint64_t, 100> prev_price;
		std::array<int, 100> prev_dir;
	};

	struct market_agg_state
	{
		double pl;
		uint64_t num_enters;
	};

	struct node_agg_state
	{
		uint64_t config_index;
		std::array<double, TOTAL_NUM_CONFIGS_DUAL> num_enters;
		std::array<double, TOTAL_NUM_CONFIGS_DUAL> pls;
	};

	struct result
	{
		std::array<double, TOTAL_NUM_CONFIGS_DUAL> num_enters;
		std::array<double, TOTAL_NUM_CONFIGS_DUAL> pls;
	};

	const worker_state zero_worker_state = {
		.entered = false,
		.prev_price = {0},
		.prev_dir = {0},
	};

	const market_agg_state zero_market_agg_state = {
		.pl = 0,
		.num_enters = 0,
	};

	const node_agg_state zero_node_agg_state = {
		.config_index = 0,
		.num_enters = {0},
		.pls = {0},
	};

	analyser<worker_state, market_agg_state, node_agg_state, result> _analyser;

	static auto predicate(const janus::meta_view& meta, const janus::stats& stats) -> bool
	{
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
		if (state.entered || market.state() != betfair::market_state::OPEN ||
		    market.inplay())
			return true;

		const config_dual& conf = configs_dual[node_agg_state.config_index];

		uint64_t start_timestamp = meta.market_start_timestamp();
		uint64_t market_timestamp = market.last_timestamp();

		if (market_timestamp >= start_timestamp)
			return true;

		uint64_t diff = start_timestamp - market_timestamp;

		if (diff > conf.pre_post_ms)
			return true;

		if (market.traded_vol() < conf.min_vol)
			return true;

		uint64_t fav_id = 0;
		uint64_t fav_id_2nd = 0;

		uint64_t fav_index = 0;
		// uint64_t fav_index_2nd = 0;

		uint64_t fav_price_index = betfair::NUM_PRICES;
		uint64_t num_favs = 0;

		for (uint64_t i = 0; i < market.runners().size(); i++) {
			auto& runner = market[i];
			if (runner.state() != betfair::runner_state::ACTIVE)
				continue;

			uint64_t price_index = runner.ladder().best_atl().first;
			if (betfair::price_range::index_to_pricex100(price_index) >
			    MAX_RUNNER_PRICEX100_DUAL)
				continue;

			if (price_index < fav_price_index) {
				num_favs = 1;
				fav_price_index = price_index;
				fav_index = i;
				fav_id = runner.id();
			} else if (price_index == fav_price_index) {
				// fav_index_2nd = i;
				fav_id_2nd = runner.id();
				num_favs++;
			}
		}

		if (num_favs >= 2 && fav_price_index >= conf.num_ticks) {
			int prev_dir1 = state.prev_dir[fav_index];

			uint64_t back_id;
			uint64_t lay_id;

			// TODO(lorenzo): Consider more complicated situations
			// like both steaming etc.

			if (prev_dir1 <= 0) {
				// #1 steaming or not observed change.
				back_id = fav_id;
				lay_id = fav_id_2nd;
			} else {
				// #2 steaming
				back_id = fav_id_2nd;
				lay_id = fav_id;
			}

			if (conf.both || !conf.reverse) {
				sim.add_bet(back_id, 1.01, STAKE_SIZE_DOUBLE, true);
				sim.add_bet(back_id,
					    betfair::price_range::index_to_price(fav_price_index -
										 conf.num_ticks),
					    STAKE_SIZE_DOUBLE, false);
				// sim.hedge(back_id, betfair::price_range::index_to_price(
				//			   fav_price_index - conf.num_ticks));
			}

			// TODO(lorenzo): Gappy markets?
			if (conf.both || conf.reverse) {
				sim.add_bet(lay_id, 1000, STAKE_SIZE_DOUBLE, false);
				sim.add_bet(lay_id,
					    betfair::price_range::index_to_price(fav_price_index +
										 conf.num_ticks),
					    STAKE_SIZE_DOUBLE, true);
				// sim.hedge(lay_id, betfair::price_range::index_to_price(
				//			  fav_price_index + conf.num_ticks));
			}

			state.entered = true;
		}

		// TODO(lorenzo): Duplication.
		for (uint64_t i = 0; i < market.runners().size(); i++) {
			auto& runner = market[i];
			if (runner.state() != betfair::runner_state::ACTIVE)
				continue;

			uint64_t price_index = runner.ladder().best_atl().first;
			if (betfair::price_range::index_to_pricex100(price_index) >
			    MAX_RUNNER_PRICEX100_DUAL)
				continue;

			uint64_t prev = state.prev_price[i];
			if (prev != price_index && prev > 0) {
				if (price_index > prev)
					state.prev_dir[i] = 1;
				else
					state.prev_dir[i] = -1;
			}

			state.prev_price[i] = price_index;
		}

		return true;
	}

	static auto market_reducer(int core, const worker_state& worker_state, janus::sim& sim,
				   betfair::market& market, bool worker_aborted,
				   market_agg_state& state, spdlog::logger* logger) -> bool
	{
		state.pl += sim.pl();
		if (worker_state.entered)
			state.num_enters++;
		return true;
	}

	static auto node_reducer(int core, const market_agg_state& market_agg_state,
				 bool market_reducer_aborted, node_agg_state& state,
				 spdlog::logger* logger) -> bool
	{
		state.pls[state.config_index] = market_agg_state.pl;
		state.num_enters[state.config_index] = market_agg_state.num_enters;
		state.config_index++;

		return state.config_index < TOTAL_NUM_CONFIGS_DUAL;
	}

	static auto reducer(const std::vector<node_agg_state>& states) -> result
	{
		result ret = {
			.num_enters = {0},
			.pls = {0},
		};

		for (const auto& state : states) {
			for (uint64_t i = 0; i < TOTAL_NUM_CONFIGS_DUAL; i++) {
				ret.pls[i] += state.pls[i];
				ret.num_enters[i] += state.num_enters[i];
			}
		}

		return ret;
	}
};
} // namespace janus::apollo
