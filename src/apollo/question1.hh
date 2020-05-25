#pragma once

#include "janus.hh"

#include <array>
#include <cstdint>
#include <iostream>
#include <vector>

//#define DUMP_CONFIG

/*
 *
 *  1. How often does _any_ runner with starting odds of <= 12 come in by 10
 *     ticks or more in the 5 minutes before post + time before inplay based on
 *     best ATB (excluding removals) with minimum market traded volume of 30k?
 */

namespace janus::apollo
{
struct config_question
{
	uint64_t start_pre_post_ms;
	double min_market_vol;
	uint64_t max_runner_pricex100;
	bool check_back;
	uint64_t threshold_ticks;
};

static constexpr uint64_t NUM_START_PRE_POST_MS = 3;
const static std::array<uint64_t, NUM_MAX_LOSS_TICKS> start_pre_post_ms_params = {
	5 * 60 * 1000, 3 * 60 * 1000, 60 * 1000};

static constexpr uint64_t NUM_MIN_MARKET_VOL = 3;
const static std::array<double, NUM_MIN_MARKET_VOL> min_market_vol_params = {50000, 30000, 15000};

static constexpr uint64_t NUM_BACK_LAY_QUESTION = 2;

static constexpr uint64_t NUM_THRESHOLD_TICKS = 4;
const static std::array<uint64_t, NUM_THRESHOLD_TICKS> threshold_ticks_params = {20, 15, 10, 5};

static constexpr uint64_t TOTAL_NUM_CONFIGS_QUESTION =
	NUM_START_PRE_POST_MS * NUM_MIN_MARKET_VOL * NUM_BACK_LAY_QUESTION * NUM_THRESHOLD_TICKS;

static std::array<config_question, TOTAL_NUM_CONFIGS_QUESTION> configs_question;

static inline void print_config_question(config_question& conf, uint64_t& index)
{
	std::cout << index << "\t" << conf.start_pre_post_ms << "\t" << conf.min_market_vol << "\t"
		  << conf.max_runner_pricex100 << "\t";
	if (conf.check_back)
		std::cout << "BACK"
			  << "\t";
	else
		std::cout << "LAY"
			  << "\t";

	std::cout << conf.threshold_ticks << std::endl;
}

static inline void init_configs_question5(config_question& conf, uint64_t& index)
{
#ifdef DUMP_CONFIG
	print_config_question(conf, index);
#endif

	configs_question[index++] = conf;
}

static inline void init_configs_question4(config_question& conf, uint64_t& index)
{
	for (uint64_t i = 0; i < NUM_THRESHOLD_TICKS; i++) {
		conf.threshold_ticks = threshold_ticks_params[i];
		init_configs_question5(conf, index);
	}
}

static inline void init_configs_question3(config_question& conf, uint64_t& index)
{
	conf.check_back = false;
	init_configs_question4(conf, index);

	conf.check_back = true;
	init_configs_question4(conf, index);
}

static inline void init_configs_question2(config_question& conf, uint64_t& index)
{
	for (uint64_t i = 0; i < NUM_MIN_MARKET_VOL; i++) {
		conf.min_market_vol = min_market_vol_params[i];
		init_configs_question3(conf, index);
	}
}

static inline void init_configs_question()
{
	uint64_t index = 0;

	for (uint64_t i = 0; i < NUM_START_PRE_POST_MS; i++) {
		config_question conf = {
			.max_runner_pricex100 = 1200, // NOLINT: Not magical.
		};
		conf.start_pre_post_ms = start_pre_post_ms_params[i];
		init_configs_question2(conf, index);
	}
}

class question1
{
public:
	static constexpr uint64_t MAX_RUNNERS = 50;

	question1()
		: _analyser(predicate, update_worker, market_reducer, node_reducer, reducer,
			    zero_worker_state, zero_market_agg_state, zero_node_agg_state)
	{
	}

	void run()
	{
		const janus::config& config = janus::parse_config();
		auto res = _analyser.run(config);

		for (uint64_t i = 0; i < TOTAL_NUM_CONFIGS_QUESTION; i++) {
			std::cout << i << "\t" << res.num_exceeded[i] << "\t"
				  << res.favs_exceeded[i] << "\t" << res.init_favs_exceeded[i]
				  << std::endl;
		}
	}

private:
	struct worker_state
	{
		bool started;
		bool any_exceeded;
		bool fav_exceeded;
		bool init_fav_exceeded;
		uint64_t init_fav_id;
		uint64_t num_removed;
		std::array<uint64_t, MAX_RUNNERS> init_prices;
		std::array<bool, MAX_RUNNERS> exceeded;
	};

	struct market_agg_state
	{
		uint64_t num_exceeded;
		uint64_t favs_exceeded;
		uint64_t init_favs_exceeded;
	};

	struct node_agg_state
	{
		uint64_t config_index;
		std::array<uint64_t, TOTAL_NUM_CONFIGS_QUESTION> num_exceeded;
		std::array<uint64_t, TOTAL_NUM_CONFIGS_QUESTION> favs_exceeded;
		std::array<uint64_t, TOTAL_NUM_CONFIGS_QUESTION> init_favs_exceeded;
	};

	struct result
	{
		std::array<uint64_t, TOTAL_NUM_CONFIGS_QUESTION> num_exceeded;
		std::array<uint64_t, TOTAL_NUM_CONFIGS_QUESTION> favs_exceeded;
		std::array<uint64_t, TOTAL_NUM_CONFIGS_QUESTION> init_favs_exceeded;
	};

	const worker_state zero_worker_state = {
		.started = false,
		.any_exceeded = false,
		.fav_exceeded = false,
		.init_fav_exceeded = false,
		.init_fav_id = 0,
		.num_removed = 0,
		.init_prices = {0},
		.exceeded = {0},
	};

	const market_agg_state zero_market_agg_state = {
		.num_exceeded = 0,
		.favs_exceeded = 0,
		.init_favs_exceeded = 0,
	};

	const node_agg_state zero_node_agg_state = {
		.config_index = 0,
		.num_exceeded = {0},
		.favs_exceeded = {0},
		.init_favs_exceeded = {0},
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
		if (market.state() != betfair::market_state::OPEN || market.inplay())
			return true;

		config_question& conf = configs_question[node_agg_state.config_index];

		uint64_t start_timestamp = meta.market_start_timestamp();
		uint64_t timestamp = market.last_timestamp();

		if (!state.started) {
			// Something's not right if we've missed the start!
			if (timestamp > start_timestamp)
				return true;

			if (market.traded_vol() < conf.min_market_vol)
				return true;

			uint64_t diff = start_timestamp - timestamp;
			if (diff > conf.start_pre_post_ms)
				return true;

			uint64_t min_price = 350;
			uint64_t fav_id = 0;

			auto& runners = market.runners();
			for (uint64_t i = 0; i < runners.size(); i++) {
				auto& runner = runners[i];
				if (runner.state() == betfair::runner_state::REMOVED) {
					state.num_removed++;
					continue;
				} else if (runner.state() != betfair::runner_state::ACTIVE) {
					continue;
				}

				uint64_t price_index = runner.ladder().best_atb().first;
				if (betfair::price_range::index_to_pricex100(price_index) >
				    conf.max_runner_pricex100)
					continue;

				if (price_index < min_price) {
					min_price = price_index;
					fav_id = runner.id();
				}

				state.init_prices[i] = price_index;
			}

			state.init_fav_id = fav_id;
			state.started = true;

			return true;
		}

		// Now we check :)

		uint64_t num_removed = 0;
		auto& runners = market.runners();

		uint64_t min_seen = 350;
		uint64_t min_exceeded = 350;

		for (uint64_t i = 0; i < runners.size(); i++) {
			auto& runner = runners[i];

			if (runner.state() == betfair::runner_state::REMOVED) {
				num_removed++;
				continue;
			} else if (runner.state() != betfair::runner_state::ACTIVE) {
				continue;
			}

			uint64_t init_price = state.init_prices[i];
			// If we couldn't get an init price then just ignore the
			// runner.
			if (init_price == 0)
				continue;

			uint64_t curr_price = runner.ladder().best_atb().first;
			bool steamed = curr_price < init_price;
			if ((!conf.check_back && steamed) || (conf.check_back && !steamed)) {
				// Reset so we get _any_ X tick move.
				state.init_prices[i] = curr_price;
				continue;
			}

			uint64_t delta;
			if (steamed)
				delta = init_price - curr_price;
			else
				delta = curr_price - init_price;

			if (curr_price < min_seen)
				min_seen = curr_price;

			if (delta > conf.threshold_ticks) {
				state.exceeded[i] = true;
				state.any_exceeded = true;

				if (runner.id() == state.init_fav_id)
					state.init_fav_exceeded = true;

				if (curr_price < min_exceeded)
					min_exceeded = curr_price;
			}
		}

		if (min_seen != 350 && min_seen == min_exceeded)
			state.fav_exceeded = true;

		// If we have more removals than we started with, a runner has
		// been removed in the last 5 mins and our results are invalid,
		// abort.
		if (num_removed > state.num_removed) {
			state.any_exceeded = false;
			state.fav_exceeded = false;
			state.init_fav_exceeded = false;
			for (uint64_t i = 0; i < runners.size(); i++) {
				state.exceeded[i] = false;
			}

			return false;
		}

		return true;
	}

	static auto market_reducer(int core, const worker_state& worker_state, janus::sim& sim,
				   bool worker_aborted, market_agg_state& state,
				   spdlog::logger* logger) -> bool
	{
		if (worker_state.any_exceeded)
			state.num_exceeded++;
		if (worker_state.fav_exceeded)
			state.favs_exceeded++;
		if (worker_state.init_fav_exceeded)
			state.init_favs_exceeded++;

		return true;
	}

	static auto node_reducer(int core, const market_agg_state& market_agg_state,
				 bool market_reducer_aborted, node_agg_state& state,
				 spdlog::logger* logger) -> bool
	{
		state.num_exceeded[state.config_index] = market_agg_state.num_exceeded;
		state.favs_exceeded[state.config_index] = market_agg_state.favs_exceeded;
		state.init_favs_exceeded[state.config_index] = market_agg_state.init_favs_exceeded;
		state.config_index++;

		return state.config_index < TOTAL_NUM_CONFIGS_QUESTION;
	}

	static auto reducer(const std::vector<node_agg_state>& states) -> result
	{
		result ret = {
			.num_exceeded = {0},
			.favs_exceeded = {0},
			.init_favs_exceeded = {0},
		};

		for (auto& state : states) {
			for (uint64_t i = 0; i < TOTAL_NUM_CONFIGS_QUESTION; i++) {
				ret.num_exceeded[i] += state.num_exceeded[i];
				ret.favs_exceeded[i] += state.favs_exceeded[i];
				ret.init_favs_exceeded[i] += state.init_favs_exceeded[i];
			}
		}

		return ret;
	}
};
} // namespace janus::apollo
