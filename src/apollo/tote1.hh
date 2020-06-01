#pragma once

#include "janus.hh"

#include <array>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

//#define DUMP_CONFIG
//#define PRINT_PL

// TODO(lorenzo): Terrible mess, first draft stuff!

namespace janus::apollo::tote1
{
static constexpr double STAKE_SIZE = 500;

static constexpr uint64_t NUM_PRE_POST_MS = 3;
const static std::array<uint64_t, NUM_PRE_POST_MS> pre_post_ms_params = {5 * 60 * 1000,
									 3 * 60 * 1000, 60 * 1000};

static constexpr uint64_t NUM_MULT = 3;
const static std::array<double, NUM_MULT> mult_params = {0.3, 0.4, 0.5};

static constexpr uint64_t NUM_LAY_MULT = 5;
const static std::array<double, NUM_LAY_MULT> lay_mult_params = {0, 0.9, 0.8, 0.7, 0.6};

static constexpr uint64_t TOTAL_NUM_CONFIGS = NUM_PRE_POST_MS * NUM_MULT * NUM_LAY_MULT;

struct config
{
	uint64_t pre_post_ms; // Point at which pre-post we start tracking.
	double mult;          // Tote multiplier.
	double lay_mult;      // Lay multiplier (0 == tote mult).
};

static std::array<config, TOTAL_NUM_CONFIGS> configs;

static inline void print_config(uint64_t index, config& conf)
{
	std::cout << index << "\t" << conf.pre_post_ms << "\t" << conf.mult << "\t" << conf.lay_mult
		  << std::endl;
}

static inline void init_configs4(config& conf, uint64_t& index)
{
#ifdef DUMP_CONFIG
	print_config(index, conf);
#endif

	configs[index++] = conf;
}

static inline void init_configs3(config& conf, uint64_t& index)
{
	for (uint64_t i = 0; i < NUM_LAY_MULT; i++) {
		conf.lay_mult = lay_mult_params[i];
		init_configs4(conf, index);
	}
}

static inline void init_configs2(config& conf, uint64_t& index)
{
	for (uint64_t i = 0; i < NUM_MULT; i++) {
		conf.mult = mult_params[i];
		init_configs3(conf, index);
	}
}

static inline void init_configs()
{
	uint64_t index = 0;

	for (uint64_t i = 0; i < NUM_PRE_POST_MS; i++) {
		config conf = {0};
		conf.pre_post_ms = pre_post_ms_params[i];
		init_configs2(conf, index);
	}

	if (index != TOTAL_NUM_CONFIGS)
		throw std::runtime_error(std::string("Unexpected index of ") +
					 std::to_string(index) + " expected " +
					 std::to_string(TOTAL_NUM_CONFIGS));
}

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

		result res = _analyser.run(config);
		for (uint64_t i = 0; i < TOTAL_NUM_CONFIGS; i++) {
			std::cout << res.pls[i] << "\t" << res.num_enters[i] << "\t" << i
				  << std::endl;
		}
	}

private:
	struct worker_state
	{
		bool entered;
		uint64_t enter_timestamp;
		uint64_t market_id;
		uint64_t fav_id;
		uint64_t fav_price_index;
		double lay_price;
	};

	struct market_agg_state
	{
		uint64_t num_enters;
		double pl;
	};

	struct node_agg_state
	{
		uint64_t config_index;
		std::array<double, TOTAL_NUM_CONFIGS> pls;
		std::array<uint64_t, TOTAL_NUM_CONFIGS> num_enters;
	};

	struct result
	{
		std::array<double, TOTAL_NUM_CONFIGS> pls;
		std::array<uint64_t, TOTAL_NUM_CONFIGS> num_enters;
	};

	const worker_state zero_worker_state = {
		.entered = false,
		.enter_timestamp = 0,
		.market_id = 0,
		.fav_id = 0,
		.fav_price_index = 0,
	};

	const market_agg_state zero_market_agg_state = {
		.num_enters = 0,
		.pl = 0,
	};

	const node_agg_state zero_node_agg_state = {
		.config_index = 0,
		.pls = {0},
		.num_enters = {0},
	};

	analyser<worker_state, market_agg_state, node_agg_state, result> _analyser;

	static auto predicate(const janus::meta_view& meta, const janus::stats& stats) -> bool
	{
		// We only want horse races.
		if (meta.event_type_id() != 7)
			return false;

		if (meta.event_country_code() != "GB")
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
		if (market.state() != betfair::market_state::OPEN || state.entered)
			return true;

		uint64_t timestamp = market.last_timestamp();
		uint64_t start = meta.market_start_timestamp();
		if (timestamp > start) {
			sim.hedge();
			return true;
		}

		const config& conf = configs[node_agg_state.config_index];
		uint64_t diff = start - timestamp;
		if (diff > conf.pre_post_ms)
			return true;

		uint64_t fav_id = 0;
		uint64_t fav_price_index = 350;
		double fav_traded_vol = 0;

		auto& runners = market.runners();
		for (uint64_t i = 0; i < runners.size(); i++) {
			auto& runner = runners[i];
			if (runner.state() != betfair::runner_state::ACTIVE)
				continue;

			uint64_t price_index = runner.ladder().best_atl().first;
			if (price_index < fav_price_index) {
				fav_id = runner.id();
				fav_price_index = price_index;
				fav_traded_vol = runner.traded_vol();
			}
		}

		if (fav_price_index == 350)
			return true;

		if (fav_traded_vol == 0)
			return true;

		double tote = market.traded_vol() / fav_traded_vol;
		if (tote == 0)
			return true;
		double price = betfair::price_range::index_to_price(fav_price_index);
		if (tote > price)
			return true;

		double mult = (price - tote) / tote;
		if (mult < conf.mult)
			return true;

		// Enter.

		sim.add_bet(fav_id, 1.01, STAKE_SIZE, true);
		// TODO(lorenzo): Use something better than instantiating price
		// range each time.
		betfair::price_range range;

		double lay = 0;
		if (conf.lay_mult > 0) {
			lay = conf.lay_mult * price;
			sim.hedge(fav_id, lay);
		}

		sim.hedge(fav_id, fav_price_index - 5);

		state.market_id = market.id();
		state.enter_timestamp = timestamp;
		state.fav_id = fav_id;
		state.fav_price_index = fav_price_index;
		state.lay_price = lay;
		state.entered = true;
		return true;
	}

	static auto market_reducer(int core, const worker_state& worker_state, janus::sim& sim,
				   betfair::market& market, bool worker_aborted,
				   market_agg_state& state, spdlog::logger* logger) -> bool
	{
		double pl = sim.pl();
		if (worker_state.entered && pl != 0) {
			state.num_enters++;
			state.pl += pl;

#ifdef PRINT_PL
			betfair::runner* runner = market.find_runner(worker_state.fav_id);
			double sp = runner == nullptr ? 0 : runner->sp();

			logger->info(
				"{}\t{}\t{}\t{}\t{}\t{}\t{}", core, worker_state.market_id,
				worker_state.enter_timestamp,
				betfair::price_range::index_to_price(worker_state.fav_price_index),
				worker_state.lay_price, sp, pl);
#endif
		}

		return true;
	}

	static auto node_reducer(int core, const market_agg_state& market_agg_state,
				 bool market_reducer_aborted, node_agg_state& state,
				 spdlog::logger* logger) -> bool
	{
		state.pls[state.config_index] = market_agg_state.pl;
		state.num_enters[state.config_index] = market_agg_state.num_enters;
		state.config_index++;

		return state.config_index < TOTAL_NUM_CONFIGS;
	}

	static auto reducer(const std::vector<node_agg_state>& states) -> result
	{
		result ret = {.pls = {0}, .num_enters = {0}};

		for (const auto& state : states) {
			for (uint64_t i = 0; i < TOTAL_NUM_CONFIGS; i++) {
				ret.pls[i] += state.pls[i];
				ret.num_enters[i] += state.num_enters[i];
			}
		}

		return ret;
	}
};
} // namespace janus::apollo::tote1
