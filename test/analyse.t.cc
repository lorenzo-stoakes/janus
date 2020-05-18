#include "janus.hh"

#include "test_util.hh"

#include <array>
#include <cstdint>
#include <gtest/gtest.h>
#include <unordered_set>
#include <vector>

namespace
{
// Test that basic functionality of analyse code works correctly.
TEST(analyse_test, basic)
{
	janus::config config = {
		.json_data_root = "../test/test-json",
		.binary_data_root = "../test/test-analyse-binary",
	};

	std::unordered_set<uint64_t> id_set = {170462852, 170462855, 170462859,
					       170462863, 170462866, 170462892,
					       170462893, 170462894, 170462895};

	uint64_t seen_markets = 0;
	uint64_t unknown_markets = 0;

	auto predicate = [&](const janus::meta_view& meta, const janus::stats& stats) -> bool {
		uint64_t id = meta.market_id();

		if (id_set.contains(id))
			seen_markets++;
		else
			unknown_markets++;

		bool went_inplay = (stats.flags & janus::stats_flags::WENT_INPLAY) ==
				   janus::stats_flags::WENT_INPLAY;

		if (id >= 170462892)
			EXPECT_FALSE(went_inplay);
		else
			EXPECT_TRUE(went_inplay);

		return true;
	};

	struct worker_state
	{
		uint64_t market_id;
		int core;
		int num_updates;
		double traded_vol;
		bool failed;
	};

	const worker_state zero_worker_state = {
		.core = -1,
	};

	auto update_worker = [&](int core, const janus::betfair::market& market,
				 const janus::sim& sim, worker_state& state,
				 spdlog::logger* logger) -> bool {
		state.num_updates++;
		state.market_id = market.id();
		state.traded_vol = market.traded_vol();

		if (state.core != -1 && core != state.core) {
			logger->error("Core {}: Market {}: Unexpected core change from {}", core,
				      market.id(), state.core);
			state.failed = true;
		}
		state.core = core;

		return true;
	};

	struct market_agg_state
	{
		std::vector<worker_state> workers;
		bool failed;
	};

	const market_agg_state zero_market_agg_state = {
		.failed = false,
	};

	auto market_reducer = [&](int core, const worker_state& worker_state, const janus::sim& sim,
				  bool worker_aborted, market_agg_state& state,
				  spdlog::logger* logger) -> bool {
		state.workers.push_back(worker_state);
		if (worker_state.failed) {
			logger->error("Core {}: market reducer failed!", core);
			state.failed = true;
		}

		if (worker_aborted) {
			logger->error("Core {}: Worker aborted?!", core);
			state.failed = true;
		}

		return true;
	};

	struct node_agg_state
	{
		int num_iters;
		std::vector<worker_state> workers;
		bool failed;
	};

	const node_agg_state zero_node_agg_state = {
		.num_iters = 0,
		.failed = false,
	};

	const uint64_t NUM_ITERS = 10;

	auto node_reducer = [&](int core, const market_agg_state& market_agg_state,
				bool market_reducer_aborted, node_agg_state& state,
				spdlog::logger* logger) -> bool {
		if (market_reducer_aborted) {
			logger->error("Core {}: Market reducer aborted?!", core);
			state.failed = true;
		}

		if (state.num_iters == NUM_ITERS)
			return false;

		if (state.num_iters == 0)
			state.workers = market_agg_state.workers;

		if (market_agg_state.failed)
			state.failed = true;

		state.num_iters++;
		return true;
	};

	struct result
	{
		int num_iters;
		std::vector<worker_state> workers;
		bool failed;
	};

	auto reducer = [&](const std::vector<node_agg_state>& states) -> result {
		result ret = {
			.num_iters = 0,
			.failed = false,
		};

		for (const auto& state : states) {
			if (state.failed)
				ret.failed = true;

			ret.num_iters += state.num_iters;
			for (const auto& worker : state.workers) {
				ret.workers.push_back(worker);
			}
		}

		return ret;
	};

	janus::analyser<worker_state, market_agg_state, node_agg_state, result> a(
		predicate, update_worker, market_reducer, node_reducer, reducer, zero_worker_state,
		zero_market_agg_state, zero_node_agg_state);
	result res = a.run(config, 3);

	EXPECT_EQ(seen_markets, id_set.size());
	ASSERT_EQ(unknown_markets, 0);

	EXPECT_FALSE(res.failed);
	EXPECT_EQ(res.num_iters, 30);

	ASSERT_EQ(res.workers.size(), 9);
	for (const auto& worker : res.workers) {
		ASSERT_TRUE(id_set.contains(worker.market_id));
		switch (worker.market_id) {
		case 170462852:
			EXPECT_EQ(worker.num_updates, 19851);
			EXPECT_DOUBLE_EQ(round_2dp(worker.traded_vol), 64034.97);
			break;
		case 170462855:
			EXPECT_EQ(worker.num_updates, 21868);
			EXPECT_DOUBLE_EQ(round_2dp(worker.traded_vol), 39810.30);
			break;
		case 170462859:
			EXPECT_EQ(worker.num_updates, 14528);
			EXPECT_DOUBLE_EQ(round_2dp(worker.traded_vol), 49683.60);
			break;
		case 170462863:
			EXPECT_EQ(worker.num_updates, 26779);
			EXPECT_DOUBLE_EQ(round_2dp(worker.traded_vol), 77970.46);
			break;
		case 170462866:
			EXPECT_EQ(worker.num_updates, 34789);
			EXPECT_DOUBLE_EQ(round_2dp(worker.traded_vol), 34093.09);
			break;
		case 170462892:
			EXPECT_EQ(worker.num_updates, 29122);
			EXPECT_DOUBLE_EQ(round_2dp(worker.traded_vol), 32303.18);
			break;
		case 170462893:
			EXPECT_EQ(worker.num_updates, 22679);
			EXPECT_DOUBLE_EQ(round_2dp(worker.traded_vol), 78655.02);
			break;
		case 170462894:
			EXPECT_EQ(worker.num_updates, 18316);
			EXPECT_DOUBLE_EQ(round_2dp(worker.traded_vol), 32131.79);
			break;
		case 170462895:
			EXPECT_EQ(worker.num_updates, 14376);
			EXPECT_DOUBLE_EQ(round_2dp(worker.traded_vol), 34531.72);
			break;
		}
	}
}
} // namespace
