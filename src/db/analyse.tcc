#pragma once

#include "janus.hh"

#include "spdlog/sinks/stdout_color_sinks.h"
#include "spdlog/spdlog.h"
#include <chrono>
#include <memory>
#include <stdexcept>
#include <thread>
#include <vector>

namespace janus
{
template<typename TWorkerState, typename TMarketAggState, typename TNodeAggState, typename TResult>
void analyser<TWorkerState, TMarketAggState, TNodeAggState, TResult>::thread_fn(
	const config& config, int core, channel<bool>& load_chan,
	channel<TNodeAggState>& result_chan, uint64_t offset, uint64_t num_markets)
{
	// TODO(lorenzo): Big mess, needs refactoring. First draft staff!

	std::shared_ptr<spdlog::logger> logger = spdlog::get("thread");

	// If we have no markets to analyse, abort.
	if (num_markets == 0) {
		logger->error("No markets to analyse, aborting!");
		load_chan.send(false);
		return;
	}

	std::vector<dynamic_buffer> bufs;
	// Load data.
	try {
		for (uint64_t i = offset; i < offset + num_markets; i++) {
			uint64_t id = _meta_views[i].market_id();
			bufs.emplace_back(read_market_updates(config, id));
		}
	} catch (std::exception& e) {
		logger->error("Core {}: got error {} on data read aborting!", core, e.what());

		load_chan.send(false);
		return;
	}

	load_chan.send(true);

	// Wait to start analysis.
	load_chan.receive();

	TNodeAggState node_agg_state = _zero_node_agg_state;

	betfair::price_range range;
	betfair::universe<1> universe;

	auto start_time = std::chrono::steady_clock::now();

	uint64_t num_iters = 0;
	while (true) {
		num_iters++;

		bool market_agg_aborted = false;

		// State that is aggregated across ALL markets.
		TMarketAggState market_agg_state = _zero_market_agg_state;
		for (uint64_t i = 0; i < num_markets; i++) {
			dynamic_buffer& buf = bufs[i];
			buf.reset_read();
			universe.clear();

			const meta_view& meta = _meta_views[offset + i];
			uint64_t market_id = meta.market_id();
			universe.apply_update(janus::make_market_id_update(market_id));

			// Get first set of data. First timestamp will be prior
			// to first market state data.
			uint64_t next = 0;
			bool first = true;
			bool abort_first = false;
			while (buf.read_offset() != buf.size()) {
				auto& u = buf.read<janus::update>();
				if (u.type == update_type::TIMESTAMP) {
					if (first) {
						first = false;
					} else {
						next = get_update_timestamp(u);
						break;
					}
				}
				try {
					universe.apply_update(u);
				} catch (janus::universe_apply_error& e) {
					logger->error(
						"Core {}: Market {}: Got error {} on market parse, skipping! [1]",
						core, market_id, e.what());

					// If we can't apply an update in this
					// market we should just abort analysing it.
					abort_first = true;
					break;
				}
			}
			if (abort_first || next == 0)
				continue;

			betfair::market& market = universe.markets()[0];
			sim sim(range, market);
			TWorkerState state = _zero_worker_state;
			// If we can't even update our very first state then
			// just skip this market.
			if (!_update_worker(core, meta, market, sim, node_agg_state, state,
					    logger.get()))
				continue;

			try {
				universe.apply_update(make_timestamp_update(next));
			} catch (janus::universe_apply_error& e) {
				logger->error(
					"Core {}: Market {}: {} - Error apply first timestamp?? Skipping!",
					core, market_id, e.what());
				continue;
			}

			bool worker_aborted = false;
			bool went_inplay = false;

			// Now iterate through rest of market timeline.
			while (buf.read_offset() != buf.size()) {
				auto& u = buf.read<janus::update>();

				// We skip inplay updates currently.
				if (u.type == update_type::TIMESTAMP && !went_inplay) {
					if (market.inplay()) {
						sim.update();
						sim.cancel_all();
						went_inplay = true;
					} else if (!_update_worker(core, meta, market, sim,
								   node_agg_state, state,
								   logger.get())) {
						worker_aborted = true;
						sim.update();
						break;
					}
				}

				try {
					universe.apply_update(u);
				} catch (janus::universe_apply_error& e) {
					logger->error(
						"Core {}: Market {}: got error {} on market parse, skipping! [2]",
						core, market_id, e.what());

					// If we can't apply an update in this
					// market we should just abort analysing
					// it. Indicate worker aborted also.
					worker_aborted = true;
					break;
				}
			} // updates

			// Perform final update to pick up winner.
			sim.update();

			// Per-market reducer.
			if (!_market_reducer(core, state, sim, market, worker_aborted,
					     market_agg_state, logger.get())) {
				market_agg_aborted = true;
				break;
			}
		} // markets
		// Reducer across all markets.
		if (!_node_reducer(core, market_agg_state, market_agg_aborted, node_agg_state,
				   logger.get()))
			break;
	} // iterations

	auto stop_time = std::chrono::steady_clock::now();
	auto duration_ms =
		std::chrono::duration_cast<std::chrono::milliseconds>(stop_time - start_time)
			.count();
	logger->info("Core {}: Executed {} iterations over {} markets taking {}ms", core, num_iters,
		     num_markets, duration_ms);

	logger->info("Core {}: Sending result data...", core);
	result_chan.send(node_agg_state);
	logger->info("Core {}: Sent!", core);
}

template<typename TWorkerState, typename TMarketAggState, typename TNodeAggState, typename TResult>
auto analyser<TWorkerState, TMarketAggState, TNodeAggState, TResult>::run(const config& config,
									  int req_cores) -> TResult
{
	if (req_cores == 0)
		throw std::runtime_error("Can't run with 0 cores!");

	// TODO(lorenzo): Maybe shouldn't be using a thread-based logger?
	spdlog::stdout_color_mt("thread");

	spdlog::info("Reading metadata...");

	std::vector<meta_view> metas = read_all_metadata(config, _meta_dyn_buf);
	_meta_views.reserve(metas.size());

	spdlog::info("Processing {} market metadata/stats against predicate...", metas.size());

	uint64_t num_missing_stats = 0;
	// Apply predicate to all meta views and stats.
	for (const auto& view : metas) {
		// We need stats to process a market.
		if (!have_market_stats(config, view.market_id())) {
			num_missing_stats++;
			continue;
		}

		const stats stats = read_market_stats(config, view.market_id());
		if (_predicate(view, stats))
			_meta_views.emplace_back(view);
	}
	uint64_t total_num_markets = _meta_views.size();
	if (total_num_markets == 0)
		throw std::runtime_error("Filtered out all markets!");

	spdlog::info("Filtered {} markets from {} ({} were missing stats)", total_num_markets,
		     metas.size(), num_missing_stats);

	uint64_t num_cores;
	if (req_cores < 0)
		num_cores = std::thread::hardware_concurrency();
	else
		num_cores = static_cast<uint64_t>(req_cores);
	if (total_num_markets < num_cores)
		num_cores = total_num_markets;
	std::vector<std::thread> threads(num_cores);
	std::vector<channel<bool>> load_chans(num_cores);
	std::vector<channel<TNodeAggState>> result_chans(num_cores);
	uint64_t markets_per_core = total_num_markets / num_cores;
	spdlog::info("Will use {} cores which will each analyse {} markets each", num_cores,
		     markets_per_core);

	// Start threads and load data in each in order that the allocations are
	// local to each core.
	uint64_t offset = 0;
	for (uint64_t core = 0; core < num_cores; core++) {
		uint64_t num_markets;
		if (core == num_cores - 1)
			num_markets = total_num_markets - offset;
		else
			num_markets = markets_per_core;

		channel<bool>& load_chan = load_chans[core];
		threads[core] = std::thread([&] {
			thread_fn(config, core, load_chan, result_chans[core], offset, num_markets);
		});

		::cpu_set_t cpuset;
		CPU_ZERO(&cpuset);
		CPU_SET(core, &cpuset);
		int err_num = ::pthread_setaffinity_np(threads[core].native_handle(),
						       sizeof(cpu_set_t), &cpuset);
		if (err_num != 0)
			throw std::runtime_error(
				std::string("Unable to set core affinity on core ") +
				std::to_string(core) + " error " + std::to_string(err_num));

		if (!load_chan.receive())
			throw std::runtime_error(std::string("Unable to load data on core ") +
						 std::to_string(core));

		spdlog::info("Core {} loaded {} markets.", core, num_markets);
		offset += num_markets;
	}

	// Kick off worker threads.
	spdlog::info("Starting worker threads...");
	for (uint64_t core = 0; core < num_cores; core++) {
		load_chans[core].send(true);
	}
	spdlog::info("All {} cores started!", num_cores);

	std::vector<TNodeAggState> node_aggs(num_cores);
	for (uint64_t core = 0; core < num_cores; core++) {
		node_aggs.emplace_back(result_chans[core].receive());
		spdlog::info("Core {} received data!", core);
	}

	spdlog::info("Data from {} cores received! Joining all threads...", num_cores);
	for (uint64_t core = 0; core < num_cores; core++) {
		threads[core].join();
	}

	spdlog::info("All threads complete, reducing aggregate data...");
	return _reducer(node_aggs);
}
} // namespace janus
