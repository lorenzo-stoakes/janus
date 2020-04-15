#pragma once

#include "dynamic_buffer.hh"
#include "price_range.hh"

#include <cstdint>
#include <utility>

namespace janus
{
// Types of update we can receive from the market.
enum class update_type : uint32_t
{
	// Declarations.
	TIMESTAMP,
	MARKET_ID,
	RUNNER_ID,

	// Market actions.
	MARKET_CLEAR,
	MARKET_OPEN,
	MARKET_CLOSE,
	MARKET_SUSPEND,
	MARKET_INPLAY,

	// Market data.
	MARKET_TRADED_VOL,

	// Runner actions.
	RUNNER_REMOVAL,

	// Runner data.
	RUNNER_TRADED_VOL,
	RUNNER_LTP,
	RUNNER_MATCHED,
	RUNNER_UNMATCHED_ATL,
	RUNNER_UNMATCHED_ATB,

	// Outcome.
	RUNNER_SP,
	RUNNER_WON,
};

// Raw update structure, we keep it the same size for all updates to make it
// easy to process a number of them in an array.
struct update
{
	update_type type;
	uint32_t key;
	union
	{
		uint64_t u;
		double d;
	} value;
};
static_assert(sizeof(update) == 16); // NOLINT: Not magical.

// Generate a new timestamp update object.
static inline auto make_timestamp_update(uint64_t timestamp) -> const update
{
	return update{
		.type = update_type::TIMESTAMP,
		.value = {.u = timestamp},
	};
}

// Retrieve timestamp from a timestamp update object.
// Note that we don't assert the type is correct, we assume the caller has done
// this already.
static inline auto get_update_timestamp(const update& update) -> uint64_t
{
	return update.value.u;
}

// Generate a new market ID update object.
static inline auto make_market_id_update(uint64_t market_id) -> const update
{
	return update{
		.type = update_type::MARKET_ID,
		.value = {.u = market_id},
	};
}

// Retrieve the market ID from a market ID update object.
// Note that we don't assert the type is correct, we assume the caller has done
// this already.
static inline auto get_update_market_id(const update& update) -> uint64_t
{
	return update.value.u;
}

// Generate a new runner ID update object.
static inline auto make_runner_id_update(uint64_t runner_id) -> const update
{
	return update{
		.type = update_type::RUNNER_ID,
		.value = {.u = runner_id},
	};
}

// Retrieve the runner ID from a runner ID update object.
// Note that we don't assert the type is correct, we assume the caller has done
// this already.
static inline auto get_update_runner_id(const update& update) -> uint64_t
{
	return update.value.u;
}

// Generate a new market clear update object.
static inline auto make_market_clear_update() -> const update
{
	return update{
		.type = update_type::MARKET_CLEAR,
	};
}

// Generate a new market traded volume update object.
static inline auto make_market_traded_vol_update(double vol) -> const update
{
	return update{
		.type = update_type::MARKET_TRADED_VOL,
		.value = {.d = vol},
	};
}

// Retrieve market traded volume from a market traded volume update object.
// Note that we don't assert the type is correct, we assume the caller has done
// this already.
static inline auto get_update_market_traded_vol(const update& update) -> double
{
	return update.value.d;
}

// Generate a new market open update object.
static inline auto make_market_open_update() -> const update
{
	return update{
		.type = update_type::MARKET_OPEN,
	};
}

// Generate a new market close update object.
static inline auto make_market_close_update() -> const update
{
	return update{
		.type = update_type::MARKET_CLOSE,
	};
}

// Generate a new market suspend update object.
static inline auto make_market_suspend_update() -> const update
{
	return update{
		.type = update_type::MARKET_SUSPEND,
	};
}

// Generate a new market inplay update object.
static inline auto make_market_inplay_update() -> const update
{
	return update{
		.type = update_type::MARKET_INPLAY,
	};
}

// Generate a new market traded volume update object.
static inline auto make_runner_sp_update(double sp) -> const update
{
	return update{
		.type = update_type::RUNNER_SP,
		.value = {.d = sp},
	};
}

// Retrieve runner SP from a runner SP update object.
// Note that we don't assert the type is correct, we assume the caller has done
// this already.
static inline auto get_update_runner_sp(const update& update) -> double
{
	return update.value.d;
}

// Generate a new runner removal update object.
static inline auto make_runner_removal_update(double adj_factor) -> const update
{
	return update{
		.type = update_type::RUNNER_REMOVAL,
		.value = {.d = adj_factor},
	};
}

// Retrieve runner adjustment factor from a runner removal update object.
// Note that we don't assert the type is correct, we assume the caller has done
// this already.
static inline auto get_update_runner_adj_factor(const update& update) -> double
{
	return update.value.d;
}

// Generate a new runner won update object.
static inline auto make_runner_won_update() -> const update
{
	return update{
		.type = update_type::RUNNER_WON,
	};
}

// Generate a new runner traded volume object.
static inline auto make_runner_traded_vol_update(double traded_vol) -> const update
{
	return update{
		.type = update_type::RUNNER_TRADED_VOL,
		.value = {.d = traded_vol},
	};
}

// Retrieve runner traded volume factor from a runner traded volume update
// object.
// Note that we don't assert the type is correct, we assume the caller has done
// this already.
static inline auto get_update_runner_traded_vol(const update& update) -> double
{
	return update.value.d;
}

// Generate a new runner LTP update object.
static inline auto make_runner_ltp_update(uint64_t price_index) -> const update
{
	return update{
		.type = update_type::RUNNER_LTP,
		.value = {.u = price_index},
	};
}

// Retrieve runner LTP price index from a runner LTP update object.
// Note that we don't assert the type is correct, we assume the caller has done
// this already.
static inline auto get_update_runner_ltp(const update& update) -> uint64_t
{
	return update.value.u;
}

// Generate a new runner matched update object.
static inline auto make_runner_matched_update(uint32_t price_index, double vol) -> const update
{
	return update{
		.type = update_type::RUNNER_MATCHED,
		.key = price_index,
		.value = {.d = vol},
	};
}

// Retrieve runner matched price index, volume from a runner matched update object.
// Note that we don't assert the type is correct, we assume the caller has done
// this already.
static inline auto get_update_runner_matched(const update& update) -> std::pair<uint64_t, double>
{
	return std::make_pair(update.key, update.value.d);
}

// Generate a new runner unmatched ATL update object.
static inline auto make_runner_unmatched_atl_update(uint32_t price_index, double vol)
	-> const update
{
	return update{
		.type = update_type::RUNNER_UNMATCHED_ATL,
		.key = price_index,
		.value = {.d = vol},
	};
}

// Retrieve runner unmatched ATL price index, volume from a runner unmatched ATL
// update object.
// Note that we don't assert the type is correct, we assume the caller has done
// this already.
static inline auto get_update_runner_unmatched_atl(const update& update)
	-> std::pair<uint64_t, double>
{
	return std::make_pair(update.key, update.value.d);
}

// Generate a new runner unmatched ATB update object.
static inline auto make_runner_unmatched_atb_update(uint32_t price_index, double vol)
	-> const update
{
	return update{
		.type = update_type::RUNNER_UNMATCHED_ATB,
		.key = price_index,
		.value = {.d = vol},
	};
}

// Retrieve runner unmatched ATB price index, volume from a runner unmatched ATB
// update object.
// Note that we don't assert the type is correct, we assume the caller has done
// this already.
static inline auto get_update_runner_unmatched_atb(const update& update)
	-> std::pair<uint64_t, double>
{
	return std::make_pair(update.key, update.value.d);
}

namespace betfair
{
// Stores the ongoing update state so we can determine if the update we're
// parsing is for a different market, runner and/or timestamp and generate
// update messages accordingly. We also store the filename for error-reporting
// purposes.
struct update_state
{
	const price_range* range;
	const char* filename;
	uint64_t line;

	uint64_t market_id;
	uint64_t runner_id;
	uint64_t timestamp;
};

// Parse betfair raw stream update data and update the specified dynamic buffer
// to store the data in binary file format.
//     state: State object which should be shared between invocations.
//       str: Char buffer containing JSON to parse. WILL BE MUTATED.
//      size: Size of char buffer.
//   dyn_buf: Dynamic buffer to OUTPUT data to in binary file format.
//   returns: Number of updates emitted.
auto parse_update_stream_json(update_state& state, char* str, uint64_t size,
			      dynamic_buffer& dyn_buf) -> uint64_t;
} // namespace betfair
} // namespace janus
