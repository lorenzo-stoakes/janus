#pragma once

#include "janus.hh"

#include <array>
#include <cstdint>
#include <iostream>
#include <vector>

/*
 *
 *  1. How often does _any_ runner with starting odds of <= 12 come in by 10 ticks
 *     or more in the 5 minutes before post + time before inplay based on best ATB
 *     (excluding removals) with minimum market traded volume of 30k?
 */

namespace janus::apollo
{
class question1
{
public:
	static constexpr uint64_t MAX_RUNNERS = 50;
	static constexpr uint64_t START_PRE_POST_MS = 5 * 60 * 1000; // 5 minutes before post.
	static constexpr double MIN_MARKET_VOL = 30000;              // Minimum market volume.
	static constexpr uint64_t MAX_RUNNER_PRICEX100 = 1200;       // Maximum runner price 12.
	static constexpr bool CHECK_BACK = true;                     // Check steam?
	static constexpr uint64_t THRESHOLD_TICKS = 10;              // Number of ticks.

	question1()
		: _analyser(predicate, update_worker, market_reducer, node_reducer, reducer,
			    zero_worker_state, zero_market_agg_state, zero_node_agg_state)
	{
	}

	void run()
	{
		const janus::config& config = janus::parse_config();
		auto res = _analyser.run(config);
		std::string direction = CHECK_BACK ? "steamed" : "drifted";
		std::cout << res.num_exceeded << " markets " << direction << " by "
			  << THRESHOLD_TICKS << " ticks or more." << std::endl;
	}

private:
	struct worker_state
	{
		bool started;
		bool exceeded;
		uint64_t num_removed;
		std::array<uint64_t, MAX_RUNNERS> init_prices;
	};

	struct market_agg_state
	{
		uint64_t num_exceeded;
	};

	struct node_agg_state
	{
		uint64_t num_exceeded;
	};

	struct result
	{
		uint64_t num_exceeded;
	};

	const worker_state zero_worker_state = {
		.started = false,
		.exceeded = false,
		.num_removed = 0,
		.init_prices = {0},
	};

	const market_agg_state zero_market_agg_state = {
		.num_exceeded = 0,
	};

	const node_agg_state zero_node_agg_state = {
		.num_exceeded = 0,
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
		if (state.exceeded || market.state() != betfair::market_state::OPEN ||
		    market.inplay())
			return true;

		uint64_t start_timestamp = meta.market_start_timestamp();
		uint64_t timestamp = market.last_timestamp();

		if (!state.started) {
			// Something's not right if we've missed the start!
			if (timestamp > start_timestamp)
				return true;

			if (market.traded_vol() < MIN_MARKET_VOL)
				return true;

			uint64_t diff = start_timestamp - timestamp;
			if (diff > START_PRE_POST_MS)
				return true;

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
				    MAX_RUNNER_PRICEX100)
					continue;

				state.init_prices[i] = price_index;
			}
			state.started = true;

			return true;
		}

		// Now we check :)

		uint64_t num_removed = 0;
		auto& runners = market.runners();
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
			if ((!CHECK_BACK && steamed) || (CHECK_BACK && !steamed))
				continue;

			uint64_t delta;
			if (steamed)
				delta = init_price - curr_price;
			else
				delta = curr_price - init_price;

			if (delta > THRESHOLD_TICKS) {
				state.exceeded = true;
				break;
			}
		}

		// If we have more removals than we started with, a runner has
		// been removed in the last 5 mins and our results are invalid,
		// abort.
		if (num_removed > state.num_removed) {
			state.exceeded = false;

			std::array<char, 25> iso8601_buf; // NOLINT: Not magical.
			auto timestamp_str =
				print_iso8601(&iso8601_buf[0], market.last_timestamp());

			logger->warn(
				"Core {}: Market {}: Timestamp {}: Saw {} removals previously {}, aborting.",
				core, market.id(), timestamp_str, num_removed, state.num_removed);

			return false;
		}

		return true;
	}

	static auto market_reducer(int core, const worker_state& worker_state, janus::sim& sim,
				   bool worker_aborted, market_agg_state& state,
				   spdlog::logger* logger) -> bool
	{
		if (worker_state.exceeded)
			state.num_exceeded++;

		return true;
	}

	static auto node_reducer(int core, const market_agg_state& market_agg_state,
				 bool market_reducer_aborted, node_agg_state& state,
				 spdlog::logger* logger) -> bool
	{
		state.num_exceeded = market_agg_state.num_exceeded;

		// Only 1 iteration.
		return false;
	}

	static auto reducer(const std::vector<node_agg_state>& states) -> result
	{
		result ret = {
			.num_exceeded = 0,
		};

		for (auto& state : states) {
			ret.num_exceeded += state.num_exceeded;
		}

		return ret;
	}
};
} // namespace janus::apollo
