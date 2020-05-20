#pragma once

#include "janus.hh"

#include <vector>

#include <iostream>

namespace janus::apollo
{
class basic1
{
public:
	basic1()
		: _analyser(predicate, update_worker, market_reducer, node_reducer, reducer,
			    zero_worker_state, zero_market_agg_state, zero_node_agg_state)
	{
	}

	auto run() -> double
	{
		const janus::config& config = janus::parse_config();

		return _analyser.run(config).pl;
	}

private:
	struct worker_state
	{
		double pl;
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
		.pl = 0,
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
				  const janus::betfair::market& market, janus::sim& sim,
				  const node_agg_state& node_agg_state, worker_state& state,
				  spdlog::logger* logger) -> bool
	{
		return true;
	}

	static auto market_reducer(int core, const worker_state& worker_state, janus::sim& sim,
				   bool worker_aborted, market_agg_state& state,
				   spdlog::logger* logger) -> bool
	{
		return true;
	}

	static auto node_reducer(int core, const market_agg_state& market_agg_state,
				 bool market_reducer_aborted, node_agg_state& state,
				 spdlog::logger* logger) -> bool
	{
		return false;
	}

	static auto reducer(const std::vector<node_agg_state>& states) -> result
	{
		result ret = {
			.pl = 0,
		};

		return ret;
	}
};
} // namespace janus::apollo
