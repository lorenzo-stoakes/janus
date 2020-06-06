#pragma once

#include "janus.hh"

#include <array>
#include <iostream>
#include <vector>

namespace janus::apollo::micro1
{
static constexpr uint64_t STAKE_SIZE = 500;

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
		// March 10th 2020 Southwell 15:05.
		return meta.market_id() == 169971977;

		// Leave rest of logic for informational purposes.

		// We only want horse races.
		if (meta.event_type_id() != 7)
			return false;

		// We have to have a full race.
		auto flags = stats.flags;
		if ((flags & janus::stats_flags::WENT_INPLAY) != janus::stats_flags::WENT_INPLAY ||
		    (flags & janus::stats_flags::WAS_CLOSED) != janus::stats_flags::WAS_CLOSED ||
		    (flags & janus::stats_flags::SAW_WINNER) != janus::stats_flags::SAW_WINNER) {
			std::cout << "IVG1" << std::endl;

			return false;
		}

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

		betfair::runner& capla_spirit = *market.find_runner(25334027);
		betfair::runner& intrepid_italian = *market.find_runner(25724347);

		if (market_timestamp == 1583852386739) {
			// 14:59:46

			sim.add_bet(capla_spirit.id(), 1000, STAKE_SIZE, false);
			auto& bet = sim.bets()[0];
			logger->info("Core {}: Capla Spirit {} {} @ {}, {} matched", core,
				     bet.is_back() ? "BACK" : "LAY", bet.stake(), bet.price(),
				     bet.matched());

			sim.add_bet(intrepid_italian.id(), 1.01, STAKE_SIZE, true);

			auto& bet2 = sim.bets()[1];
			logger->info("Core {}: Intrepid Italian {} {} @ {}, {} matched", core,
				     bet2.is_back() ? "BACK" : "LAY", bet2.stake(), bet2.price(),
				     bet2.matched());
		} else if (market_timestamp == 1583852484548) {
			// 15:01:24

			sim.hedge(capla_spirit.id());

			auto& bet = sim.bets()[2];
			logger->info("Core {}: [HEDGE] Capla Spirit {} {} @ {}, {} matched", core,
				     bet.is_back() ? "BACK" : "LAY", bet.stake(), bet.price(),
				     bet.matched());

			sim.hedge(intrepid_italian.id());

			auto& bet2 = sim.bets()[3];
			logger->info("Core {}: [HEDGE] Intrepid Italian {} {} @ {}, {} matched",
				     core, bet2.is_back() ? "BACK" : "LAY", bet2.stake(),
				     bet2.price(), bet2.matched());
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
