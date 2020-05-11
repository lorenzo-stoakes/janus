#include "janus.hh"

#include <cstdint>

namespace janus
{
// Read through market updates in specified dynamic buffer, updating interval
// stats in specified stats object.
// TODO(lorenzo): This is too much code for 1 function and there is too much
// indentation.
static void add_interval_stats(stats& stats, dynamic_buffer& dyn_buf)
{
	struct interval_state
	{
		uint64_t num_updates;
		uint64_t num_timestamps;
		uint64_t sum_intervals_ms;
		uint64_t worst_interval_ms;
	};

	std::array<interval_state, NUM_STATS_INTERVALS> pre_post_states{};
	std::array<interval_state, NUM_STATS_INTERVALS> pre_inplay_states{};
	interval_state inplay_state{};

	std::array<bool, NUM_STATS_INTERVALS> post_started{};
	std::array<bool, NUM_STATS_INTERVALS> inplay_started{};

	bool calc_post = (stats.flags & stats_flags::HAVE_METADATA) == stats_flags::HAVE_METADATA;
	bool calc_inplay = (stats.flags & stats_flags::WENT_INPLAY) == stats_flags::WENT_INPLAY;

	bool inplay = false;
	uint64_t timestamp = 0;
	while (dyn_buf.read_offset() != dyn_buf.size()) {
		update u = dyn_buf.read<update>();
		if (u.type == update_type::TIMESTAMP) {
			uint64_t next = get_update_timestamp(u);
			// If we're at the first timestamp there's nothing to do.
			if (timestamp == 0) {
				timestamp = next;
				continue;
			}
			// If we encounter a duplicate timestamp, ignore the
			// update.
			if (timestamp == next)
				continue;

			uint64_t delta_ms = next - timestamp;

			for (uint64_t i = 0; i < NUM_STATS_INTERVALS; i++) {
				if (calc_post && post_started[i]) {
					interval_state& state = pre_post_states[i];
					state.num_timestamps++;
					state.sum_intervals_ms += delta_ms;
					if (delta_ms > state.worst_interval_ms)
						state.worst_interval_ms = delta_ms;
				}

				if (calc_inplay && inplay_started[i]) {
					interval_state& state = pre_inplay_states[i];
					// TODO(lorenzo): Duplication.
					state.num_timestamps++;
					state.sum_intervals_ms += delta_ms;
					if (delta_ms > state.worst_interval_ms)
						state.worst_interval_ms = delta_ms;
				}
			}

			if (inplay) {
				// TODO(lorenzo): More duplication!
				inplay_state.num_timestamps++;
				inplay_state.sum_intervals_ms += delta_ms;
				if (delta_ms > inplay_state.worst_interval_ms)
					inplay_state.worst_interval_ms = delta_ms;
			}

			// We set things started after we first enter the
			// interval so we look at the first delta in the
			// interval rather than the first delta prior to it.

			// Determine which intervals have started to accumulate stats.
			for (uint64_t i = 0; i < NUM_STATS_INTERVALS; i++) {
				uint64_t interval_mins = INTERVAL_PERIODS_MINS[i];
				uint64_t interval_ms = interval_mins * MS_PER_MIN;

				if (calc_post && next < stats.start_timestamp) {
					uint64_t diff = stats.start_timestamp - next;
					if (diff <= interval_ms)
						post_started[i] = true;
				} else {
					post_started[i] = false;
				}

				if (calc_inplay && next < stats.inplay_timestamp) {
					uint64_t diff = stats.inplay_timestamp - next;
					if (diff <= interval_ms)
						inplay_started[i] = true;
				} else {
					inplay_started[i] = false;
				}
			}

			timestamp = next;
		} else if (u.type == update_type::MARKET_INPLAY) {
			inplay = true;
		}

		// Track number of updates too.
		for (uint64_t i = 0; i < NUM_STATS_INTERVALS; i++) {
			if (calc_post && post_started[i]) {
				interval_state& state = pre_post_states[i];
				state.num_updates++;
			}

			if (calc_inplay && inplay_started[i]) {
				interval_state& state = pre_inplay_states[i];
				state.num_updates++;
			}
		}
		if (inplay)
			inplay_state.num_updates++;
	}

	for (uint64_t i = 0; i < NUM_STATS_INTERVALS; i++) {
		if (calc_post) {
			interval_state& state = pre_post_states[i];
			interval_stats& target = stats.pre_post_intervals[i];
			target.num_updates = state.num_updates;
			target.worst_update_interval_ms = state.worst_interval_ms;
			if (state.num_timestamps > 0) {
				double avg = static_cast<double>(state.sum_intervals_ms);
				avg /= static_cast<double>(state.num_timestamps);
				target.mean_update_interval_ms = avg;
			} else {
				target.mean_update_interval_ms = 0;
			}
		}
		if (calc_inplay) {
			// TODO(lorenzo): Duplication.
			interval_state& state = pre_inplay_states[i];
			interval_stats& target = stats.pre_inplay_intervals[i];
			target.num_updates = state.num_updates;
			target.worst_update_interval_ms = state.worst_interval_ms;
			if (state.num_timestamps > 0) {
				double avg = static_cast<double>(state.sum_intervals_ms);
				avg /= static_cast<double>(state.num_timestamps);
				target.mean_update_interval_ms = avg;
			} else {
				target.mean_update_interval_ms = 0;
			}
		}
	}

	if (inplay) {
		// TODO(lorenzo): Duplication.
		interval_stats& target = stats.inplay_intervals;
		target.num_updates = inplay_state.num_updates;
		target.worst_update_interval_ms = inplay_state.worst_interval_ms;
		if (inplay_state.num_timestamps > 0) {
			double avg = static_cast<double>(inplay_state.sum_intervals_ms);
			avg /= static_cast<double>(inplay_state.num_timestamps);
			target.mean_update_interval_ms = avg;
		} else {
			target.mean_update_interval_ms = 0;
		}
	}
}

auto generate_stats(meta_view* meta, dynamic_buffer& dyn_buf) -> stats
{
	stats ret = {
		.flags = stats_flags::DEFAULT,
		.num_updates = dyn_buf.size() / sizeof(update),
	};

	bool have_meta = meta != nullptr;
	if (have_meta) {
		ret.flags |= stats_flags::HAVE_METADATA;

		// If there's no metadata we work this out from runner IDs we
		// encounter below.
		ret.num_runners = meta->runners().size();

		ret.start_timestamp = meta->market_start_timestamp();
	}

	std::array<uint64_t, betfair::MAX_RUNNERS> seen_runners{};
	uint64_t num_seen_runners = 0;
	auto have_seen_runner = [&seen_runners, &num_seen_runners](uint64_t id) -> bool {
		for (uint64_t i = 0; i < num_seen_runners; i++) {
			if (id == seen_runners[i])
				return true;
		}
		return false;
	};

	std::array<uint64_t, betfair::MAX_RUNNERS> seen_removals{};
	uint64_t num_seen_removals = 0;
	auto have_seen_removal = [&seen_removals, &num_seen_removals](uint64_t id) -> bool {
		for (uint64_t i = 0; i < num_seen_removals; i++) {
			if (id == seen_removals[i])
				return true;
		}
		return false;
	};

	dyn_buf.reset_read();
	uint64_t timestamp = 0;
	bool first = true;
	uint64_t runner_id = 0;
	while (dyn_buf.read_offset() != dyn_buf.size()) {
		update u = dyn_buf.read<update>();

		switch (u.type) {
		case update_type::TIMESTAMP:
			timestamp = get_update_timestamp(u);
			if (first) {
				ret.first_timestamp = timestamp;
				first = false;
			}
			break;
		case update_type::MARKET_INPLAY:
			ret.flags |= stats_flags::WENT_INPLAY;
			ret.inplay_timestamp = timestamp;
			break;
		case update_type::MARKET_CLOSE:
			ret.flags |= stats_flags::WAS_CLOSED;
			break;
		case update_type::RUNNER_ID:
			runner_id = get_update_runner_id(u);
			if (!have_seen_runner(runner_id))
				seen_runners[num_seen_runners++] = runner_id;
			break;
		case update_type::RUNNER_SP:
			ret.flags |= stats_flags::SAW_SP;
			break;
		case update_type::RUNNER_WON:
			ret.flags |= stats_flags::SAW_WINNER;
			ret.winner_runner_id = runner_id;
			break;
		case update_type::RUNNER_REMOVAL:
			if (!have_seen_removal(runner_id))
				seen_removals[num_seen_removals++] = runner_id;
			break;
		default:
			// We don't care about the other updates.
			break;
		}
	}
	ret.last_timestamp = timestamp;
	if (have_meta) {
		if (timestamp > ret.start_timestamp)
			ret.flags |= stats_flags::PAST_POST;
	} else {
		ret.num_runners = num_seen_runners;
	}

	ret.num_removals = num_seen_removals;

	// If we don't know the post time and we didn't go inplay, we can't
	// gather interval statistics.
	bool gone_inplay = (ret.flags & stats_flags::WENT_INPLAY) == stats_flags::WENT_INPLAY;
	if (!have_meta && !gone_inplay)
		return ret;

	// We have to perform a second pass since we had to determine inplay
	// time.
	dyn_buf.reset_read();
	add_interval_stats(ret, dyn_buf);

	return ret;
}
} // namespace janus
