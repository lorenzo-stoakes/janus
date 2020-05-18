#pragma once

#include "dynamic_array.hh"
#include "dynamic_buffer.hh"
#include "market.hh"
#include "meta.hh"
#include "sim.hh"
#include "spdlog/spdlog.h"
#include "stats.hh"

#include <cstdint>
#include <functional>
#include <vector>

namespace janus
{
static constexpr uint64_t MAX_ANALYSE_CORES = 256;
static constexpr uint64_t MAX_ANALYSE_METADATA_BYTES = 250'000'000;

template<typename TWorkerState, typename TMarketAggState, typename TNodeAggState, typename TResult>
class analyser
{
public:
	using predicate_fn_t = std::function<bool(const meta_view&, const stats&)>;
	using update_worker_fn_t = std::function<bool(int, const betfair::market&, const sim&,
						      TWorkerState&, spdlog::logger*)>;
	using market_reducer_fn_t =
		std::function<bool(int, const TWorkerState&, const janus::sim& sim, bool,
				   TMarketAggState&, spdlog::logger*)>;
	using node_reducer_fn_t = std::function<bool(int, const TMarketAggState&, bool,
						     TNodeAggState&, spdlog::logger*)>;
	using reducer_fn_t = std::function<TResult(const std::vector<TNodeAggState>&)>;

	analyser(const predicate_fn_t& predicate, const update_worker_fn_t& update_worker,
		 const market_reducer_fn_t& market_reducer, const node_reducer_fn_t& node_reducer,
		 const reducer_fn_t& reducer, const TWorkerState& zero_worker_state,
		 const TMarketAggState& zero_market_agg_state,
		 const TNodeAggState& zero_node_agg_state)
		: _predicate{predicate},
		  _update_worker{update_worker},
		  _market_reducer{market_reducer},
		  _node_reducer{node_reducer},
		  _reducer{reducer},
		  _zero_worker_state{zero_worker_state},
		  _zero_market_agg_state{zero_market_agg_state},
		  _zero_node_agg_state{zero_node_agg_state},
		  _meta_dyn_buf{MAX_ANALYSE_METADATA_BYTES}
	{
	}

	// Execute the run against markets. Limit to specified number of cores
	// or all available if unspecified.
	auto run(const config& config, int num_cores = -1) -> TResult;

private:
	const predicate_fn_t _predicate;
	const update_worker_fn_t _update_worker;
	const market_reducer_fn_t _market_reducer;
	const node_reducer_fn_t _node_reducer;
	const reducer_fn_t _reducer;
	const TWorkerState _zero_worker_state;
	const TMarketAggState _zero_market_agg_state;
	const TNodeAggState _zero_node_agg_state;

	dynamic_buffer _meta_dyn_buf;
	std::vector<meta_view> _meta_views;

	void thread_fn(const config& config, int core, channel<bool>& load_chan,
		       channel<TNodeAggState>& result_chan, uint64_t offset, uint64_t num_markets);
};
} // namespace janus

#include "db/analyse.tcc"
