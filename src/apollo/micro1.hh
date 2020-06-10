#pragma once

#include "janus.hh"

#include <array>
#include <iostream>
#include <vector>

namespace janus::apollo::micro1
{
static constexpr uint64_t STAKE_SIZE = 500;

// Southwell 5f Nov Stks, 2019-08-26 16:45:00
static constexpr uint64_t MARKET_ID = 161743011;
// ~16:41:35
static constexpr uint64_t ENTRY_TIMESTAMP = 1566834095006;
// ~16:43:26
static constexpr uint64_t EXIT_TIMESTAMP = 1566834206839;
// Bhangra
static constexpr uint64_t BACK_RUNNER_ID = 19056873;

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

		auto res = _analyser.run(config);
		std::cout << res.pl << std::endl;
	}

private:
	struct worker_state
	{
		bool bet;
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
		.bet = false,
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
		return meta.market_id() == MARKET_ID;

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

		uint64_t start_timestamp = meta.market_start_timestamp();
		if (market_timestamp > start_timestamp)
			return true;

		betfair::runner& bhangra = *market.find_runner(BACK_RUNNER_ID);

		if (market_timestamp == ENTRY_TIMESTAMP) {
			sim.add_bet(bhangra.id(), 1.01, STAKE_SIZE, true);

			auto& bet = sim.bets()[0];
			logger->info("Core {}: Bhangra {} {} @ {}, {} matched", core,
				     bet.is_back() ? "BACK" : "LAY", bet.stake(), bet.price(),
				     bet.matched());
		} else if (market_timestamp == EXIT_TIMESTAMP) {
			sim.hedge();

			auto& bet = sim.bets()[1];
			logger->info("Core {}: [HEDGE] Bhangra {} {} @ {}, {} matched", core,
				     bet.is_back() ? "BACK" : "LAY", bet.stake(), bet.price(),
				     bet.matched());
		}

		return true;
	}

	static auto market_reducer(int core, const worker_state& worker_state, janus::sim& sim,
				   betfair::market& market, bool worker_aborted,
				   market_agg_state& state, spdlog::logger* logger) -> bool
	{
		double pl = sim.pl();
		state.pl += pl;

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
} // namespace janus::apollo::micro1
