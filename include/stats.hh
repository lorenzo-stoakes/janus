#pragma once

#include <array>
#include <cstdint>

namespace janus
{
// Periods prior to post/inplay over which interval statistics are generated, in
// minutes.
static const std::array<int, 6> INTERVAL_PERIODS_MINS = {60, 30, 10, 5, 3, 1};

// Represents 'market is X' information for a market.
enum class stats_flags : uint64_t
{
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
	uint64_t median_update_interval_ms;
	uint64_t mean_update_interval_ms;
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
	double winner_sp;

	std::array<interval_stats, 6> pre_post_intervals;
	std::array<interval_stats, 6> pre_inplay_intervals;
	interval_stats inplay_intervals;
};
} // namespace janus
