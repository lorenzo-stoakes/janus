#pragma once

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
	MARKET_INPLAY,
	MARKET_SUSPEND,
	MARKET_UNSUSPEND,
	MARKET_CLOSE,

	// Market data.
	MARKET_TRADED_VOL,

	// Runner data.
	RUNNER_TRADED_VOL,
	RUNNER_LTP,
	RUNNER_MATCHED,
	RUNNER_UNMATCHED_ATL,
	RUNNER_UNMATCHED_ATB,

	// Runner actions.
	RUNNER_REMOVAL,

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
static_assert(sizeof(update) == 16);

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

// Generate a new market clear update object.
static inline auto make_market_clear_update() -> const update
{
	return update{
		.type = update_type::MARKET_CLEAR,
	};
}

// Stores the ongoing update state so we can determine if the update we're
// parsing is for a different market, runner and/or timestamp and generate
// update messages accordingly. We also store the filename for error-reporting
// purposes.
struct update_state
{
	const char* filename;
	uint64_t line;

	uint64_t market_id;
	uint64_t runner_id;
	uint64_t timestamp;
};

namespace betfair
{
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
