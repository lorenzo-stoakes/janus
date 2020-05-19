#pragma once

#include <array>
#include <cstdint>

#include "dynamic_buffer.hh"

namespace janus
{
// Periods prior to post/inplay over which interval statistics are generated, in
// minutes.

static constexpr uint64_t NUM_STATS_INTERVALS = 6;
static const std::array<int, NUM_STATS_INTERVALS> INTERVAL_PERIODS_MINS = {60, 30, 10, 5, 3, 1};

// Represents the interval periods, indexes into the interval stats arrays.
enum class stats_interval : uint64_t
{
	HOUR,
	HALF_HOUR,
	TEN_MINS,
	FIVE_MINS,
	THREE_MINS,
	ONE_MIN,
};

// Represents 'market is X' information for a market.
enum class stats_flags : uint64_t
{
	DEFAULT = 0,
	HAVE_METADATA = 1 << 0,
	PAST_POST = 1 << 1,
	WENT_INPLAY = 1 << 2,
	WAS_CLOSED = 1 << 3,
	SAW_SP = 1 << 4,
	SAW_WINNER = 1 << 5,
};

// Permit flags to use the | operator.
static inline auto operator|(stats_flags a, stats_flags b) -> stats_flags
{
	uint64_t ret = static_cast<uint64_t>(a) | static_cast<uint64_t>(b);
	return static_cast<stats_flags>(ret);
}

// Permit flags to use the |= operator.
static inline auto operator|=(stats_flags& a, stats_flags b) -> stats_flags&
{
	a = a | b;
	return a;
}

// Permit flags to use the & operator.
static inline auto operator&(stats_flags a, stats_flags b) -> stats_flags
{
	uint64_t ret = static_cast<uint64_t>(a) & static_cast<uint64_t>(b);
	return static_cast<stats_flags>(ret);
}

// Represents data over a specific interval.
struct interval_stats
{
	uint64_t num_updates;
	double mean_update_interval_ms;
	uint64_t worst_update_interval_ms;
};

// Statistics describing a market.
struct stats
{
	stats_flags flags;
	uint64_t num_updates;
	uint64_t num_runners;
	uint64_t num_removals;
	uint64_t first_timestamp;
	uint64_t start_timestamp;
	uint64_t inplay_timestamp;
	uint64_t last_timestamp;
	uint64_t winner_runner_id;

	std::array<interval_stats, NUM_STATS_INTERVALS> pre_post_intervals;
	std::array<interval_stats, NUM_STATS_INTERVALS> pre_inplay_intervals;
	interval_stats inplay_intervals;
};

// Generate statistics from market updates contained within a dynamic buffer and
// an optional metadata view.
auto generate_stats(meta_view* meta, dynamic_buffer& dyn_buf) -> stats;
} // namespace janus
