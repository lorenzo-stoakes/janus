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

namespace janus::apollo
{
static constexpr double STAKE_SIZE = 500;
static constexpr uint64_t FAV_NUM_HISTORY = 100;
static constexpr uint64_t MIN_STABLE_COUNT = 10;
static constexpr uint64_t MAX_STABLE_DELTA = 1;

enum class exit_strategy
{
	NONE,
	PROFIT_TICKS,
	MAX_DURATION,
	UNTIL_STABLE,
};

// Node blank = ~7ms, ~600 markets/node = ~5s per iteration.

static constexpr uint64_t NUM_MAX_LOSS_TICKS = 1;
const static std::array<uint64_t, NUM_MAX_LOSS_TICKS> max_loss_ticks_params = {3};

static constexpr uint64_t NUM_PRE_POST_SECS = 1;
const static std::array<uint64_t, NUM_PRE_POST_SECS> pre_post_secs_params = {180};
static constexpr uint64_t NUM_MAX_AFTER_POST_SECS = 1;
const static std::array<uint64_t, NUM_MAX_AFTER_POST_SECS> max_after_post_secs_params = {0};

static constexpr uint64_t NUM_MIN_VOLS = 1;
const static std::array<double, NUM_MIN_VOLS> min_vol_params = {50000};

static constexpr uint64_t NUM_TRIGGER_NUM_TICKS = 1;
const static std::array<uint64_t, NUM_TRIGGER_NUM_TICKS> trigger_num_ticks_params = {2};

static constexpr uint64_t NUM_INTERVAL_MS = 1;
const static std::array<uint64_t, NUM_INTERVAL_MS> interval_ms_params = {1000};

static constexpr uint64_t NUM_MAX_FAV_PRICEX100 = 1;
const static std::array<uint64_t, NUM_MAX_FAV_PRICEX100> max_fav_pricex100_params = {0};

static constexpr uint64_t NUM_BACK_LAY = 1; // lay.

static constexpr uint64_t NUM_OPPOSE = 1;

static constexpr uint64_t NUM_EXIT_STRAT_NO_PARAMS = 1;

static constexpr uint64_t NUM_EXIT_TICKS = 1;
const static std::array<uint64_t, NUM_EXIT_TICKS> exit_ticks_params = {5};

static constexpr uint64_t TOTAL_NUM_CONFIGS =
	NUM_MAX_LOSS_TICKS * NUM_PRE_POST_SECS * NUM_MAX_AFTER_POST_SECS * NUM_MIN_VOLS *
	NUM_TRIGGER_NUM_TICKS * NUM_INTERVAL_MS * NUM_BACK_LAY * NUM_OPPOSE *
	NUM_EXIT_STRAT_NO_PARAMS * NUM_MAX_FAV_PRICEX100 * NUM_EXIT_TICKS;

struct config_catch
{
	uint64_t max_loss_ticks;      // Maximum number of ticks of loss we will accept.
	uint64_t pre_post_secs;       // Point at which pre-post we start tracking.
	uint64_t max_after_post_secs; // Max time after post for exit.
	double min_vol;               // Minimum market traded volume.
	uint64_t trigger_num_ticks;   // How many ticks before we enter.
	uint64_t interval_ms;         // Duration over which we count tick delta.
	bool back;                    // Movement down?
	bool oppose;                  // Fade?
	exit_strategy exit_strat;     // How we handle exit.
	uint64_t exit_num_ticks;      // If PROFIT_TICKS exit strat, number of ticks.
	uint64_t exit_duration_ms;    // If MAX_DURATION exit strat, max duration.
	uint64_t max_fav_pricex100;   // Maximum price of favourite *100.
	uint64_t exit_ticks;          // Number of ticks we want to exceed by.
};

static std::array<config_catch, TOTAL_NUM_CONFIGS> configs_catch;

static inline void print_config(uint64_t index, config_catch& conf)
{
	std::cout << index << "\t" << conf.max_loss_ticks << "\t" << conf.pre_post_secs << "\t"
		  << conf.max_after_post_secs << "\t" << conf.min_vol << "\t"
		  << conf.trigger_num_ticks << "\t" << conf.interval_ms << "\t";
	if (conf.back)
		std::cout << "back"
			  << "\t";
	else
		std::cout << "lay"
			  << "\t";

	if (conf.oppose)
		std::cout << "oppose"
			  << "\t";
	else
		std::cout << "no-oppose"
			  << "\t";

	switch (conf.exit_strat) {
	case exit_strategy::NONE:
		std::cout << "NONE"
			  << "\t";
		break;
	case exit_strategy::PROFIT_TICKS:
		std::cout << "PROFIT_TICKS"
			  << "\t";
		std::cout << conf.exit_num_ticks;
		break;
	case exit_strategy::MAX_DURATION:
		std::cout << "MAX_DURATION"
			  << "\t";
		std::cout << conf.exit_duration_ms;
		break;
	case exit_strategy::UNTIL_STABLE:
		std::cout << "UNTIL_STABLE"
			  << "\t";
		break;
	}

	std::cout << std::endl;
}

static inline void init_configs_catch12(config_catch& conf, uint64_t& index)
{
#ifdef DUMP_CONFIG
	print_config(index, conf);
#endif

	configs_catch[index++] = conf;
}

static inline void init_configs_catch11(config_catch& conf, uint64_t& index)
{
	for (uint64_t i = 0; i < NUM_EXIT_TICKS; i++) {
		conf.exit_ticks = exit_ticks_params[i];
		init_configs_catch12(conf, index);
	}
}

static inline void init_configs_catch10(config_catch& conf, uint64_t& index)
{
	for (uint64_t i = 0; i < NUM_MAX_FAV_PRICEX100; i++) {
		conf.max_fav_pricex100 = max_fav_pricex100_params[i];
		init_configs_catch11(conf, index);
	}
}

static inline void init_configs_catch9(config_catch& conf, uint64_t& index)
{
	conf.exit_strat = exit_strategy::NONE;
	init_configs_catch10(conf, index);
}

static inline void init_configs_catch8(config_catch& conf, uint64_t& index)
{
	conf.oppose = true;
	init_configs_catch9(conf, index);
}

static inline void init_configs_catch7(config_catch& conf, uint64_t& index)
{
	conf.back = false;
	init_configs_catch8(conf, index);
}

static inline void init_configs_catch6(config_catch& conf, uint64_t& index)
{
	for (uint64_t i = 0; i < NUM_INTERVAL_MS; i++) {
		conf.interval_ms = interval_ms_params[i];
		init_configs_catch7(conf, index);
	}
}

static inline void init_configs_catch5(config_catch& conf, uint64_t& index)
{
	for (uint64_t i = 0; i < NUM_TRIGGER_NUM_TICKS; i++) {
		conf.trigger_num_ticks = trigger_num_ticks_params[i];
		init_configs_catch6(conf, index);
	}
}

static inline void init_configs_catch4(config_catch& conf, uint64_t& index)
{
	for (uint64_t i = 0; i < NUM_MIN_VOLS; i++) {
		conf.min_vol = min_vol_params[i];
		init_configs_catch5(conf, index);
	}
}

static inline void init_configs_catch3(config_catch& conf, uint64_t& index)
{
	for (uint64_t i = 0; i < NUM_MAX_AFTER_POST_SECS; i++) {
		conf.max_after_post_secs = max_after_post_secs_params[i];
		init_configs_catch4(conf, index);
	}
}

static inline void init_configs_catch2(config_catch& conf, uint64_t& index)
{
	for (uint64_t i = 0; i < NUM_PRE_POST_SECS; i++) {
		conf.pre_post_secs = pre_post_secs_params[i];
		init_configs_catch3(conf, index);
	}
}

static inline void init_configs_catch()
{
	uint64_t index = 0;

	for (uint64_t i = 0; i < NUM_MAX_LOSS_TICKS; i++) {
		config_catch conf = {0};
		conf.max_loss_ticks = max_loss_ticks_params[i];
		init_configs_catch2(conf, index);
	}

	if (index != TOTAL_NUM_CONFIGS)
		throw std::runtime_error(std::string("Unexpected index of ") +
					 std::to_string(index) + " expected " +
					 std::to_string(TOTAL_NUM_CONFIGS));
}

class catch1
{
public:
	catch1()
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
		uint64_t market_id;

		bool entered;
		bool exited;

		uint64_t num_history;
		uint64_t fav_id;

		uint64_t enter_timestamp;
		uint64_t enter_price_index;

		uint64_t last_enter_index;
		uint64_t stable_count;

		uint64_t read_offset;
		std::array<uint64_t, FAV_NUM_HISTORY> timestamps;
		std::array<uint64_t, FAV_NUM_HISTORY> price_indexes;
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
		.market_id = 0,

		.entered = false,
		.exited = false,

		.num_history = 0,
		.fav_id = 0,

		.enter_timestamp = 0,
		.enter_price_index = 0,

		.last_enter_index = 0,
		.stable_count = 0,

		.read_offset = 0,
		.timestamps = {0},
		.price_indexes = {0},
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
		state.market_id = meta.market_id();

		if (market.state() != betfair::market_state::OPEN)
			return true;

		const config_catch& conf = configs_catch[node_agg_state.config_index];

		// We're already done.
		if (state.exited)
			return true;

		uint64_t timestamp = market.last_timestamp();

		if (state.entered) {
			// Exit.

			if (conf.exit_strat == exit_strategy::NONE)
				return true;

			bet& enter_bet = sim.bets()[0];
			if ((enter_bet.flags() & bet_flags::VOIDED) == bet_flags::VOIDED)
				return true;

			if (conf.exit_strat == exit_strategy::MAX_DURATION) {
				uint64_t diff = timestamp - state.enter_timestamp;
				if (diff < conf.exit_duration_ms)
					return true;
			}

			// TODO(lorenzo): Don't create a new price range each
			// time.
			betfair::price_range range;
			uint64_t enter_index = range.price_to_nearest_index(enter_bet.price());
			// TODO(lorenzo): Handle invalid index?

			auto* runner = market.find_runner(state.fav_id);
			// Shouldn't happen!
			if (runner == nullptr)
				return true;

			if (runner->state() != betfair::runner_state::ACTIVE)
				return true;

			bool back = enter_bet.is_back();

			uint64_t latest_index;
			// We look at bet side of spread, ignoring cost of
			// crossing the spread here.
			if (back)
				latest_index = runner->ladder().best_atl().first;
			else
				latest_index = runner->ladder().best_atb().first;

			uint64_t loss_ticks = 0;
			if (back && latest_index > enter_index)
				loss_ticks = latest_index - enter_index;
			else if (!back && latest_index < enter_index)
				loss_ticks = enter_index - latest_index;
			if (loss_ticks >= conf.max_loss_ticks) {
				state.exited = true;
				sim.hedge();
				return true;
			}

			uint64_t profit_ticks =
				back ? enter_index - latest_index : latest_index - enter_index;

			if (conf.exit_strat == exit_strategy::PROFIT_TICKS &&
			    profit_ticks < conf.exit_num_ticks)
				return true;

			if (conf.exit_strat == exit_strategy::UNTIL_STABLE) {
				uint64_t delta;
				if (latest_index > state.last_enter_index)
					delta = latest_index - state.last_enter_index;
				else
					delta = state.last_enter_index - latest_index;
				state.last_enter_index = latest_index;
				if (delta <= MAX_STABLE_DELTA)
					state.stable_count++;
				else
					state.stable_count = 0;

				if (state.stable_count < MIN_STABLE_COUNT)
					return true;
			}

			state.exited = true;
			sim.hedge();

			return true;
		}

		uint64_t fav_id = 0;
		uint64_t fav_index = 350;

		bool back = conf.back;

		// Get favourite.
		auto& runners = market.runners();
		for (uint64_t i = 0; i < runners.size(); i++) {
			auto& runner = runners[i];
			if (runner.state() != betfair::runner_state::ACTIVE)
				continue;

			uint64_t price_index;
			if (back)
				price_index = runner.ladder().best_atl().first;
			else
				price_index = runner.ladder().best_atb().first;

			if (price_index < fav_index) {
				fav_id = runner.id();
				fav_index = price_index;
			}
		}

		// Keep track of favourite timestamp, prices.

		if (state.fav_id != fav_id) {
			state.fav_id = fav_id;

			state.timestamps[0] = timestamp;
			state.price_indexes[0] = fav_index;
			state.num_history = 1;
			state.read_offset = 0;
			return true;
		}

		uint64_t num_history = state.num_history;
		if (num_history < FAV_NUM_HISTORY) {
			state.timestamps[num_history] = timestamp;
			state.price_indexes[num_history] = fav_index;
			state.num_history++;
			return true;
		}

		uint64_t read_offset = state.read_offset;
		state.timestamps[read_offset] = timestamp;
		state.price_indexes[read_offset] = fav_index;
		state.read_offset = (state.read_offset + 1) % FAV_NUM_HISTORY;

		// Entry.

		// Check traded volume.
		if (market.traded_vol() < conf.min_vol)
			return true;

		// Check pre-post secs.
		uint64_t start = meta.market_start_timestamp();
		if (timestamp > start)
			return true;
		uint64_t diff = start - timestamp;
		if (diff > conf.pre_post_secs * 1000)
			return true;

		uint64_t interval_ms = conf.interval_ms;
		uint64_t offset = state.read_offset;
		for (uint64_t i = 0; i < state.num_history; i++) {
			uint64_t ind = (offset + i) % FAV_NUM_HISTORY;

			uint64_t prev_timestamp = state.timestamps[ind];
			if (timestamp - prev_timestamp > interval_ms)
				continue;

			uint64_t price_index = state.price_indexes[ind];
			if (fav_index > price_index) { // drifted
				// If we're backing then not interested in a
				// drift.
				if (back)
					return true;

				uint64_t ticks = fav_index - price_index;
				if (ticks < conf.trigger_num_ticks)
					return true;
			} else { // steamed
				// If we're laying then not interested in a
				// steam.
				if (!back)
					return true;

				uint64_t ticks = price_index - fav_index;
				if (ticks < conf.trigger_num_ticks)
					return true;
			}

			break;
		}

		uint64_t pricex100 = betfair::price_range::index_to_pricex100(fav_index);
		if (conf.max_fav_pricex100 > 0 && pricex100 > conf.max_fav_pricex100)
			return true;

		// OK now we're at the point where we make a bet.

		bool is_back = back;
		if (conf.oppose)
			is_back = !is_back;

		if (is_back) {
			double price =
				betfair::price_range::index_to_price(fav_index + conf.exit_ticks);
			// sim.add_bet(fav_id, 1.01, STAKE_SIZE, true);
			sim.add_bet(fav_id, price, STAKE_SIZE, true);
			sim.add_bet(fav_id, betfair::price_range::index_to_price(fav_index),
				    STAKE_SIZE, false);
		} else {
			double price =
				betfair::price_range::index_to_price(fav_index - conf.exit_ticks);
			// sim.add_bet(fav_id, 1000, STAKE_SIZE, false);
			sim.add_bet(fav_id, price, STAKE_SIZE, false);
			sim.add_bet(fav_id, betfair::price_range::index_to_price(fav_index),
				    STAKE_SIZE, true);
		}

		state.entered = true;
		state.enter_timestamp = timestamp;
		state.enter_price_index = fav_index;

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

			logger->info("{}\t{}\t{}\t{}\t{}\t{}", core, worker_state.market_id,
				     worker_state.enter_timestamp,
				     betfair::price_range::index_to_price(
					     worker_state.enter_price_index),
				     sp, pl);
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
} // namespace janus::apollo
