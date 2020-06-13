#pragma once

#include "janus.hh"

#include <array>
#include <iomanip>
#include <iostream>
#include <vector>

#define USE_PRESET_MARKETS
#define PRINT_TRADES

namespace janus::apollo::steve1
{
static constexpr double STAKE_SIZE = 500;
static constexpr double MIN_MARKET_TRADED_VOL = 50000;
static constexpr uint64_t NUM_HISTORY_TICKS = 20;
static constexpr uint64_t MAX_NUM_RUNNERS = 50;
// We ignore matched volume below this for range calcs.
static constexpr double MIN_MATCHED_VOL_RANGE = 50;
static constexpr uint64_t MAX_BEFORE_POST_MS = 5 * 60 * 1000;
// Only consider runners up to price 12.
static constexpr uint64_t MAX_PRICEX100 = 1200;
// For the runner we're backing - maximum ticks above bottom of range.
static constexpr uint64_t MAX_TICKS_ABOVE_BOTTOM = 2;
// For the runner we're backing - number of ticks above bottom of range before we exit.
static constexpr uint64_t MAX_TICKS_ABOVE_BOTTOM_EXIT = 4;
// For the runner we're not backing - maximum ticks below top of range.
static constexpr uint64_t MAX_TICKS_BELOW_TOP = 3;
// Minimum ticks above X/O for a back.
static constexpr uint64_t MIN_TICKS_ABOVE_XO = 5;
// The minimum number of ticks below XO.
static constexpr uint64_t MIN_TICKS_BELOW_XO = 0;
// Minimum drop before we'll back.
static constexpr uint64_t MIN_DROP = 3;
// Minimum rise before we consider no opposition
static constexpr uint64_t MIN_RISE = 3;
// Maximum loss ticks before exit.
static constexpr uint64_t MAX_LOSS_TICKS = 4;
// MAx loss in GBP.
static constexpr double MAX_LOSS_GBP = 1000;
// Min entry price.
static constexpr uint64_t MIN_ENTER_PRICEX100 = 165;
// Max entry price.
static constexpr uint64_t MAX_ENTER_PRICEX100 = 600;

static constexpr double HEDGE_AT_PROFIT_GBP = 0;

// Southwell 5f Nov Stks, 2019-08-26 16:45:00
static constexpr uint64_t MARKET_ID = 161743011;

static uint64_t find_fav(betfair::market& market)
{
	uint64_t fav_index = MAX_NUM_RUNNERS;
	uint64_t fav_price_index = betfair::NUM_PRICES;

	auto& runners = market.runners();
	for (uint64_t i = 0; i < runners.size(); i++) {
		auto& runner = runners[i];
		if (runner.state() != betfair::runner_state::ACTIVE)
			continue;

		uint64_t price_index = runner.ladder().best_atl().first;
		if (price_index < fav_price_index) {
			fav_price_index = price_index;
			fav_index = i;
		}
	}

	return fav_index;
}

class runner_state
{
public:
	runner_state() : _history_size{0}, _history_offset{0}, _history_ticks{{0}} {}

	auto last_price_index() -> uint64_t
	{
		if (_history_size == 0)
			return 0;

		if (_history_offset == 0)
			return _history_ticks[_history_size - 1];

		return _history_ticks[_history_offset - 1];
	}

	void update(betfair::runner& runner)
	{
		if (runner.state() != betfair::runner_state::ACTIVE)
			return;

		uint64_t price_index = runner.ladder().best_atl().first;
		add_price_index(price_index);
	}

	// Get the bottom and top of the matched ladder.
	auto get_top_bottom_range(betfair::runner& runner) -> std::pair<uint64_t, uint64_t>
	{
		// By default these indicate not found.
		uint64_t bottom = betfair::NUM_PRICES;
		uint64_t top = betfair::NUM_PRICES;

		auto& ladder = runner.ladder();
		for (uint64_t i = 0; i < betfair::NUM_PRICES; i++) {
			double vol = ladder.matched(i);
			if (dz(vol) || vol < MIN_MATCHED_VOL_RANGE)
				continue;

			if (bottom == betfair::NUM_PRICES)
				bottom = i;

			top = i;
		}

		return std::make_pair(bottom, top);
	}

	auto get_max_drop_rise() -> std::pair<uint64_t, uint64_t>
	{
		if (_history_size < 3)
			return std::make_pair(0, 0);

		uint64_t max_drop = 0;
		uint64_t max_rise = 0;

		uint64_t ind = _history_offset;
		uint64_t first = _history_ticks[ind];
		ind = (ind + 1) % NUM_HISTORY_TICKS;
		uint64_t second = _history_ticks[ind];
		ind = (ind + 1) % NUM_HISTORY_TICKS;

		bool rising = second > first;
		uint64_t curr_delta;
		if (rising) {
			curr_delta = second - first;
			max_rise = curr_delta;
		} else {
			curr_delta = first - second;
			max_drop = curr_delta;
		}

		uint64_t last = second;
		for (uint64_t i = 2; i < _history_size; i++) {
			uint64_t price_index = _history_ticks[ind];

			bool was_rise = price_index > last;
			if (was_rise) {
				uint64_t delta = price_index - last;

				if (rising) {
					curr_delta += delta;
				} else {
					curr_delta = delta;
					rising = true;
				}

				if (curr_delta > max_rise)
					max_rise = curr_delta;
			} else {
				uint64_t delta = last - price_index;

				if (!rising) {
					curr_delta += delta;
				} else {
					curr_delta = delta;
					rising = false;
				}

				if (curr_delta > max_drop)
					max_drop = curr_delta;
			}

			last = price_index;
			ind = (ind + 1) % NUM_HISTORY_TICKS;
		}

		return std::make_pair(max_drop, max_rise);
	}

	auto get_ticks_above_xo(betfair::runner& runner) -> uint64_t
	{
		uint64_t price_index = runner.ladder().best_atl().first;
		return TICKS_ABOVE_XO[price_index];
	}

	auto get_ticks_below_xo(betfair::runner& runner) -> uint64_t
	{
		uint64_t price_index = runner.ladder().best_atl().first;
		return TICKS_BELOW_XO[price_index];
	}

	auto can_back(betfair::runner& runner) -> uint64_t
	{
		uint64_t price_index = runner.ladder().best_atl().first;
		uint64_t pricex100 = betfair::price_range::index_to_pricex100(price_index);

		if (pricex100 < MIN_ENTER_PRICEX100 || pricex100 > MAX_ENTER_PRICEX100)
			return false;

		auto [bottom, top] = get_top_bottom_range(runner);
		if (bottom == betfair::NUM_PRICES || top == betfair::NUM_PRICES)
			return false;

		// < bottom when in region where matched volume < minimum.
		uint64_t ticks_from_bottom = price_index < bottom ? 0 : price_index - bottom;
		if (ticks_from_bottom > MAX_TICKS_ABOVE_BOTTOM)
			return false;

		uint64_t ticks_above_xo = get_ticks_above_xo(runner);
		if (ticks_above_xo < MIN_TICKS_ABOVE_XO)
			return false;

		uint64_t ticks_below_xo = get_ticks_below_xo(runner);
		if (ticks_below_xo < MIN_TICKS_BELOW_XO)
			return false;

		auto [drop, rise] = get_max_drop_rise();
		if (rise >= drop)
			return false;

		if (drop < MIN_DROP)
			return false;

		return true;
	}

	auto opposition(betfair::runner& runner) -> bool
	{
		uint64_t price_index = runner.ladder().best_atl().first;
		auto [bottom, top] = get_top_bottom_range(runner);
		if (bottom == betfair::NUM_PRICES || top == betfair::NUM_PRICES)
			return true;

		// > top when in region where matched volume < minimum.
		uint64_t ticks_below_top = price_index > top ? 0 : top - price_index;
		if (ticks_below_top > MAX_TICKS_BELOW_TOP)
			return true;

		auto [drop, rise] = get_max_drop_rise();
		if (drop >= rise)
			return true;

		if (rise < MIN_RISE)
			return true;

		return false;
	}

	auto should_exit(betfair::market& market, uint64_t fav_index, uint64_t enter_price_index)
		-> bool
	{
		auto& runner = market[fav_index];
		if (runner.state() != betfair::runner_state::ACTIVE)
			return false;

		// Cut our losses if we've exceeded maximum loss.
		uint64_t price_index = runner.ladder().best_atl().first;
		if (price_index > enter_price_index &&
		    price_index - enter_price_index > MAX_LOSS_TICKS)
			return true;

		auto [bottom, top] = get_top_bottom_range(runner);
		if (bottom == betfair::NUM_PRICES || top == betfair::NUM_PRICES)
			return false;

		uint64_t ticks_from_bottom = price_index < bottom ? 0 : price_index - bottom;
		if (ticks_from_bottom > MAX_TICKS_ABOVE_BOTTOM_EXIT)
			return true;

		return false;
	}

private:
	static constexpr std::array<uint64_t, betfair::NUM_PRICES> TICKS_ABOVE_XO = {
		1000, 1000, 1000, 1000, 1000, 1000, 1000, 1000, 1000, 1000, 1000, 1000, 1000, 1000,
		1000, 1000, 1000, 1000, 1000, 1000, 1000, 1000, 1000, 1000, 1000, 1000, 1000, 1000,
		1000, 1000, 1000, 1000, 1000, 1000, 1000, 1000, 1000, 1000, 1000, 1000, 1000, 1000,
		1000, 1000, 1000, 1000, 1000, 1000, 1000, 1000, 1000, 1000, 1000, 1000, 1000, 1000,
		1000, 1000, 1000, 1000, 1000, 1000, 1000, 1000, 1000, 1000, 1000, 1000, 1000, 1000,
		1000, 1000, 1000, 1000, 1000, 1000, 1000, 1000, 1000, 1000, 1000, 1000, 1000, 1000,
		1000, 1000, 1000, 1000, 1000, 1000, 1000, 1000, 1000, 1000, 1000, 1000, 1000, 1000,
		1000, 0,    1,    2,    3,    4,    5,    6,    7,    8,    9,    10,   11,   12,
		13,   14,   15,   16,   17,   18,   19,   20,   21,   22,   23,   24,   25,   26,
		27,   28,   29,   30,   31,   32,   33,   34,   35,   36,   37,   38,   39,   40,
		41,   42,   43,   44,   45,   46,   47,   48,   49,   0,    1,    2,    3,    4,
		5,    6,    7,    8,    9,    10,   11,   12,   13,   14,   15,   16,   17,   18,
		19,   0,    1,    2,    3,    4,    5,    6,    7,    8,    9,    10,   11,   12,
		13,   14,   15,   16,   17,   18,   19,   0,    1,    2,    3,    4,    5,    6,
		7,    8,    9,    10,   11,   12,   13,   14,   15,   16,   17,   18,   19,   0,
		1,    2,    3,    4,    5,    6,    7,    8,    9,    10,   11,   12,   13,   14,
		15,   16,   17,   18,   19,   0,    1,    2,    3,    4,    5,    6,    7,    8,
		9,    0,    1,    2,    3,    4,    5,    6,    7,    8,    9,    0,    1,    2,
		3,    4,    5,    6,    7,    8,    9,    0,    1,    2,    3,    4,    5,    6,
		7,    8,    9,    10,   11,   12,   13,   14,   15,   16,   17,   18,   19,   20,
		21,   22,   23,   24,   25,   26,   27,   28,   29,   30,   31,   32,   33,   34,
		35,   36,   37,   38,   39,   40,   41,   42,   43,   44,   45,   46,   47,   48,
		49,   50,   51,   52,   53,   54,   55,   56,   57,   58,   59,   60,   61,   62,
		63,   64,   65,   66,   67,   68,   69,   70,   71,   72,   73,   74,   75,   76,
		77,   78,   79,   80,   81,   82,   83,   84,   85,   86,   87,   88,   89,   90};

	static constexpr std::array<uint64_t, betfair::NUM_PRICES> TICKS_BELOW_XO = {
		99,   98,   97,   96,   95,   94,   93,   92,   91,   90,   89,   88,   87,   86,
		85,   84,   83,   82,   81,   80,   79,   78,   77,   76,   75,   74,   73,   72,
		71,   70,   69,   68,   67,   66,   65,   64,   63,   62,   61,   60,   59,   58,
		57,   56,   55,   54,   53,   52,   51,   50,   49,   48,   47,   46,   45,   44,
		43,   42,   41,   40,   39,   38,   37,   36,   35,   34,   33,   32,   31,   30,
		29,   28,   27,   26,   25,   24,   23,   22,   21,   20,   19,   18,   17,   16,
		15,   14,   13,   12,   11,   10,   9,    8,    7,    6,    5,    4,    3,    2,
		1,    0,    49,   48,   47,   46,   45,   44,   43,   42,   41,   40,   39,   38,
		37,   36,   35,   34,   33,   32,   31,   30,   29,   28,   27,   26,   25,   24,
		23,   22,   21,   20,   19,   18,   17,   16,   15,   14,   13,   12,   11,   10,
		9,    8,    7,    6,    5,    4,    3,    2,    1,    0,    19,   18,   17,   16,
		15,   14,   13,   12,   11,   10,   9,    8,    7,    6,    5,    4,    3,    2,
		1,    0,    19,   18,   17,   16,   15,   14,   13,   12,   11,   10,   9,    8,
		7,    6,    5,    4,    3,    2,    1,    0,    19,   18,   17,   16,   15,   14,
		13,   12,   11,   10,   9,    8,    7,    6,    5,    4,    3,    2,    1,    0,
		19,   18,   17,   16,   15,   14,   13,   12,   11,   10,   9,    8,    7,    6,
		5,    4,    3,    2,    1,    0,    19,   18,   17,   16,   15,   14,   13,   12,
		11,   10,   9,    8,    7,    6,    5,    4,    3,    2,    1,    0,    9,    8,
		7,    6,    5,    4,    3,    2,    1,    0,    1000, 1000, 1000, 1000, 1000, 1000,
		1000, 1000, 1000, 1000, 1000, 1000, 1000, 1000, 1000, 1000, 1000, 1000, 1000, 1000,
		1000, 1000, 1000, 1000, 1000, 1000, 1000, 1000, 1000, 1000, 1000, 1000, 1000, 1000,
		1000, 1000, 1000, 1000, 1000, 1000, 1000, 1000, 1000, 1000, 1000, 1000, 1000, 1000,
		1000, 1000, 1000, 1000, 1000, 1000, 1000, 1000, 1000, 1000, 1000, 1000, 1000, 1000,
		1000, 1000, 1000, 1000, 1000, 1000, 1000, 1000, 1000, 1000, 1000, 1000, 1000, 1000,
		1000, 1000, 1000, 1000, 1000, 1000, 1000, 1000, 1000, 1000, 1000, 1000, 1000, 1000};

	void add_price_index(uint64_t price_index)
	{
		uint64_t last = last_price_index();
		if (price_index == last)
			return;

		if (_history_size < NUM_HISTORY_TICKS) {
			_history_ticks[_history_size++] = price_index;
			return;
		}

		_history_ticks[_history_offset] = price_index;
		_history_offset = (_history_offset + 1) % NUM_HISTORY_TICKS;
	}

	uint64_t _history_size;
	uint64_t _history_offset;
	std::array<uint64_t, NUM_HISTORY_TICKS> _history_ticks;
};

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

#ifdef PRINT_ORDERS
		auto res = _analyser.run(config, 1);
#else
		auto res = _analyser.run(config);
#endif
		std::cout << std::fixed << std::setprecision(2) << res.pl << std::endl;
	}

private:
	struct worker_state
	{
		bool entered;
		bool exited;
		uint64_t enter_price_index;
		uint64_t enter_index;
		uint64_t lowest_enter_price_index;
		std::array<runner_state, MAX_NUM_RUNNERS> states;
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
		.entered = false,
		.exited = false,
		.enter_price_index = 0,
		.enter_index = 0,
		.lowest_enter_price_index = 0,
		.states = {},
	};

	const market_agg_state zero_market_agg_state = {
		.pl = 0,
	};

	const node_agg_state zero_node_agg_state = {
		.pl = 0,
	};

	analyser<worker_state, market_agg_state, node_agg_state, result> _analyser;

	static constexpr uint64_t market_ids[] = {
		151516272, 170480766, 152794983, 164598348, 158249920, 156690754, 162329760,
		170355395, 168014750, 153485372, 162126856, 159834261, 169971868, 161334775,
		161861735, 154380220, 154928385, 154896860, 160724544, 155823416, 154380337,
		157988021, 166783965, 170025454, 161861720, 157315434, 161358371, 160035026,
		161184401, 160357789, 158777622, 165217137, 159299691, 161334296, 160396748,
		170595334, 160586795, 167177564, 166514291, 161152359, 170473968, 155817452,
		155851308, 155779866, 160585995, 168902625, 162025467, 162604108, 153765750,
		152764488, 169721608, 166979632, 155178813, 152607262, 160173470, 165432914,
		154959707, 151620007, 170099601, 170547391, 153164828, 156229641, 160872156,
		163468625, 162956109, 156937956, 164418946, 159851657, 161881542, 160484784,
		152241332, 156652963, 154959742, 159978681, 162201371, 169927777, 152864351,
		170473971, 152930112, 163867456, 162995248, 156229103, 152795058, 166429173,
		165044930, 155176867, 159688211, 155817487, 153285781, 156120124, 162053706,
		159520154, 161743011, 161879040, 159704309, 163617866, 165270084, 170605548,
		170553859, 159374828, 161419059, 162073428, 162673727, 162053571, 166399401,
		167524195, 152795043, 166898779, 152383644, 167638247, 161864718, 166472799,
		166854896, 156060848, 163867441, 160933103, 152521908, 158091903, 170402093,
		168799538, 160585955, 153685207, 169500936, 160527512, 160682530, 162307752,
		155850565, 154896598, 162085232, 151619987, 157843490, 160933128, 169057044,
		166711751, 162266309, 153485278, 161154225, 152735660, 153166328, 167108168,
		158215527, 158644124, 154658047, 156282699, 157932759, 161220012, 160932230,
		154495945, 152987153, 159492274, 156759144, 151619977, 166510117, 167624145,
		170576276, 159492259, 158093681, 157315485, 160585975, 154335514, 160368544,
		160176589, 162597876, 153079634, 161184548, 155393191, 157968446, 162332202,
		160839575, 170205243, 156119978, 152951051, 162478449, 159834302, 162714878,
		161334785, 159493296, 167748771, 153606424, 169927396, 170484461, 164707261,
		158145620, 155714031, 159520159, 168430952, 158249935, 159520058, 169499212,
		154605107, 160394418, 151846565, 158616385, 170424715, 159820016, 166305786,
		161742986, 158984585, 161096840, 164081137, 155851318, 152607277, 161607994,
		170595021, 159425498, 159757242, 155210221, 154256663, 152094290, 160659198,
		152930032, 154029648, 162516913, 160863276, 162238846, 166709048, 153066417,
		170181348, 152951076, 153202628};

	static auto predicate(const janus::meta_view& meta, const janus::stats& stats) -> bool
	{
#ifdef USE_PRESET_MARKETS
		uint64_t market_id = meta.market_id();
		for (uint64_t i = 0; i < sizeof(market_ids) / sizeof(uint64_t); i++) {
			if (market_id == market_ids[i]) {
				std::cout << market_id << ": " << meta.describe(true) << std::endl;
				return true;
			}
		}

		return false;
#endif

		// Ignore maidens.
		if (meta.name().find("Mdn") != std::string::npos)
			return false;

		if (meta.name().find("Nursery") != std::string::npos)
			return false;

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
		if (state.entered && state.exited)
			return true;

		if (market.state() != betfair::market_state::OPEN)
			return true;

		uint64_t market_timestamp = market.last_timestamp();
		uint64_t start_timestamp = meta.market_start_timestamp();

		uint64_t price_index = market[state.enter_index].ladder().best_atl().first;

		if (state.entered) {
			bool exit = market_timestamp > start_timestamp;
			if (price_index > state.enter_price_index) {
				double enter_price = betfair::price_range::index_to_price(
					state.enter_price_index);
				double price = betfair::price_range::index_to_price(price_index);
				double loss = STAKE_SIZE * (price - enter_price) / price;
				if (loss > MAX_LOSS_GBP) {
					exit = true;
					logger->info(
						"LOSS! Core {}: Market {}: price {} > entry {}, loss of {}, exiting!",
						core, market.id(), price, enter_price, loss);
				}
			}

			if (HEDGE_AT_PROFIT_GBP > 0 && price_index < state.enter_price_index) {
				double enter_price = betfair::price_range::index_to_price(
					state.enter_price_index);
				double price = betfair::price_range::index_to_price(price_index);
				double profit = STAKE_SIZE * (enter_price - price) / price;
				if (profit >= HEDGE_AT_PROFIT_GBP) {
					exit = true;
					logger->info(
						"PROFIT! Core {}: Market {}: price {} > entry {}, loss of {}, exiting!",
						core, market.id(), price, enter_price, profit);
				}
			}

			// Exit at post.
			if (exit) {
#ifdef PRINT_TRADES
				uint64_t price_index =
					market[state.enter_index].ladder().best_atl().first;
				char print_buf[25];
				auto timestamp_str = print_iso8601(print_buf, market_timestamp);
				logger->info(
					"Core {}: Market {}: EXIT at {} (timestamp {}) at price {}, market vol {}",
					core, market.id(), timestamp_str.data(), market_timestamp,
					betfair::price_range::index_to_price(price_index),
					market.traded_vol());
#endif
				sim.hedge();
				state.exited = true;

				return true;
			}

			return true;
		}

		auto& runners = market.runners();
		for (uint64_t i = 0; i < runners.size(); i++) {
			auto& runner = runners[i];
			state.states[i].update(runner);
		}

		if (start_timestamp - market_timestamp > MAX_BEFORE_POST_MS)
			return true;

		if (market.traded_vol() < MIN_MARKET_TRADED_VOL)
			return true;

		uint64_t fav_index = find_fav(market);
		// If we cannot find for some reason, abort.
		if (fav_index == MAX_NUM_RUNNERS)
			return true;

		// If the favourite exceeds our maximum price, abort.
		auto& fav = market[fav_index];
		uint64_t fav_price_index = fav.ladder().best_atl().first;
		if (fav_price_index > MAX_PRICEX100)
			return true;

		// Check that we can back the favourite.
		if (!state.states[fav_index].can_back(fav))
			return true;

		// Check that we don't get opposition from other credible runners.
		for (uint64_t i = 0; i < runners.size(); i++) {
			if (i == fav_index)
				continue;

			auto& runner = runners[i];
			uint64_t price_index = runner.ladder().best_atl().first;

			if (betfair::price_range::index_to_pricex100(price_index) > MAX_PRICEX100)
				continue;

			// If any opposition, abort.
			if (state.states[i].opposition(runner))
				return true;
		}

		// Now we've decided our favourite is backable and we have no
		// opposition. Time to enter.

#ifdef PRINT_TRADES
		char print_buf[25];
		auto timestamp_str = print_iso8601(print_buf, market_timestamp);
		logger->info(
			"Core {}: Market {}: Runner {}: ENTER at {} (timestamp {}) at price {}, market vol {}",
			core, market.id(), fav.id(), timestamp_str.data(), market_timestamp,
			betfair::price_range::index_to_price(fav_price_index), market.traded_vol());
#endif
		sim.add_bet(fav.id(), 1.01, STAKE_SIZE, true);

		state.entered = true;
		state.enter_price_index = fav_price_index;
		state.lowest_enter_price_index = fav_price_index;
		state.enter_index = fav_index;

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
} // namespace janus::apollo::steve1
