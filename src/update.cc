#include "janus.hh"

#include "sajson.hh"

#include <cstring>
#include <stdexcept>
#include <string>

namespace janus::betfair
{
// Obtain a prefix for error messages indicating filename and line for easier
// debugging. Potentially allocates, but we're ok with that if the exceptional
// code path is being taken.
static std::string get_error_prefix(const update_state& state)
{
	return std::string("error: ") + state.filename + ":" + std::to_string(state.line) + ": ";
}

// Check that the JSON contains an "op":"mcm" k/v pair, indicating a market
// change message.
static void check_op(const update_state& state, const sajson::value& root)
{
	sajson::value op = root.get_value_of_key(sajson::literal("op"));
	if (op.get_type() != sajson::TYPE_STRING)
		throw std::runtime_error(get_error_prefix(state) + "Missing or invalid op type");
	const char* op_str = op.as_cstring();
	uint64_t size = op.get_string_length();

	if (size != sizeof("mcm") - 1 || ::strncmp(op_str, "mcm", sizeof("mcm") - 1) != 0)
		throw std::runtime_error(get_error_prefix(state) + "Invalid op value of '" +
					 op_str + "', expect 'mcm'");
}

// Process market change update.
static uint64_t parse_mc(update_state& state, const sajson::value& mc, dynamic_buffer& dyn_buf)
{
	sajson::value id = mc.get_value_of_key(sajson::literal("id"));
	uint64_t market_id =
		janus::internal::parse_market_id(id.as_cstring(), id.get_string_length());

	uint64_t num_updates = 0;

	// If the market ID has changed from the last market ID update sent, we
	// need to send + update.
	if (state.market_id != market_id) {
		dyn_buf.add(make_market_id_update(market_id));
		num_updates++;
		state.market_id = market_id;
	}

	// "img": true indicates this is a snapshot update not a delta, so we
	// need to clear down market data cache.
	sajson::value img = mc.get_value_of_key(sajson::literal("img"));
	if (img.get_type() == sajson::TYPE_TRUE) {
		dyn_buf.add(make_market_clear_update());
		num_updates++;
	}

	return num_updates;
}

auto parse_update_stream_json(update_state& state, char* str, uint64_t size,
			      dynamic_buffer& dyn_buf) -> uint64_t
{
	sajson::document doc = janus::internal::parse_json(state.filename, str, size);
	const sajson::value& root = doc.get_root();

	check_op(state, root);

	sajson::value mcs = root.get_value_of_key(sajson::literal("mc"));
	// If there is no market change update key at all, then we have nothing to do.
	if (mcs.get_type() == sajson::TYPE_NULL)
		return 0;

	uint64_t num_mcs = mcs.get_length();
	// If there are no market change updates then we have nothing to do.
	if (num_mcs == 0)
		return 0;

	uint64_t num_updates = 0;

	uint64_t timestamp = root.get_value_of_key(sajson::literal("pt")).get_integer_value();
	// Check whether we need to send a timestamp update.
	if (timestamp != state.timestamp) {
		dyn_buf.add(make_timestamp_update(timestamp));
		num_updates++;
		state.timestamp = timestamp;
	}

	// Now process the market change updates.
	for (uint64_t i = 0; i < num_mcs; i++) {
		sajson::value mc = mcs.get_array_element(i);
		num_updates += parse_mc(state, mc, dyn_buf);
	}

	state.line++;
	return num_updates;
}
} // namespace janus::betfair
