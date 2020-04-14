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

// Parse runner definition in order to extract any non-runner or runner BSP data.
static auto parse_runner_definition(update_state& state, const sajson::value& runner_def,
				    dynamic_buffer& dyn_buf) -> uint64_t
{
	// We may duplicate data sent from here, again the reciever of this data
	// should be able to handle it being sent more than once for a runner.

	uint64_t num_updates = 0;

	// Nothing to do.
	if (runner_def.get_type() != sajson::TYPE_OBJECT)
		return 0;

	sajson::value id = runner_def.get_value_of_key(sajson::literal("id"));
	uint64_t runner_id = id.get_integer_value();

	auto send_runner_id = [&]() {
		if (state.runner_id != runner_id) {
			dyn_buf.add(make_runner_id_update(runner_id));
			num_updates++;
			state.runner_id = runner_id;
		}
	};

	sajson::value bsp = runner_def.get_value_of_key(sajson::literal("bsp"));
	if (bsp.get_type() != sajson::TYPE_NULL) {
		send_runner_id();

		double bsp_val = bsp.get_number_value();
		dyn_buf.add(make_runner_sp_update(bsp_val));
		num_updates++;
	}

	sajson::value status = runner_def.get_value_of_key(sajson::literal("status"));
	const char* status_str = "";
	uint64_t size = 0;
	if (status.get_type() != sajson::TYPE_NULL) {
		status_str = status.as_cstring();
		size = status.get_string_length();
	}

	if (size == sizeof("REMOVED") - 1 &&
	    ::strncmp(status_str, "REMOVED", sizeof("REMOVED") - 1) == 0) {
		sajson::value adj_factor =
			runner_def.get_value_of_key(sajson::literal("adjustmentFactor"));
		double adj_factor_val = adj_factor.get_number_value();

		send_runner_id();
		dyn_buf.add(make_runner_removal_update(adj_factor_val));
		num_updates++;
	} else if (size == sizeof("WINNER") - 1 &&
		   ::strncmp(status_str, "WINNER", sizeof("WINNER") - 1) == 0) {
		send_runner_id();
		dyn_buf.add(make_runner_won_update());
		num_updates++;
	}

	return num_updates;
}

// Parse market definition node and generate updates as required.
static auto parse_market_definition(update_state& state, const sajson::value& market_def,
				    dynamic_buffer& dyn_buf) -> uint64_t
{
	// Nothing to do.
	if (market_def.get_type() != sajson::TYPE_OBJECT)
		return 0;

	uint64_t num_updates = 0;

	// Handle market status updates - open/close/suspend. We may duplicate
	// status reports here (due to multiple market definition updates being
	// sent) but the receiver should be able to handle this.
	sajson::value status = market_def.get_value_of_key(sajson::literal("status"));
	if (status.get_type() == sajson::TYPE_STRING) {
		const char* status_str = status.as_cstring();
		uint64_t size = status.get_string_length();

		if (size == sizeof("OPEN") - 1 &&
		    ::strncmp(status_str, "OPEN", sizeof("OPEN") - 1) == 0) {
			dyn_buf.add(make_market_open_update());
			num_updates++;
		} else if (size == sizeof("CLOSED") - 1 &&
			   ::strncmp(status_str, "CLOSED", sizeof("CLOSED") - 1) == 0) {
			dyn_buf.add(make_market_close_update());
			num_updates++;
		} else if (size == sizeof("SUSPENDED") - 1 &&
			   ::strncmp(status_str, "SUSPENDED", sizeof("SUSPENDED") - 1) == 0) {
			dyn_buf.add(make_market_suspend_update());
			num_updates++;
		}
	}

	// Handle inplay notification. Again there might be duplicate updates
	// sent here, but the receiver should understand that inplay being sent
	// again should be ignored.
	sajson::value inplay = market_def.get_value_of_key(sajson::literal("inPlay"));
	if (inplay.get_type() == sajson::TYPE_TRUE) {
		dyn_buf.add(make_market_inplay_update());
		num_updates++;
	}

	sajson::value runner_defs = market_def.get_value_of_key(sajson::literal("runners"));
	if (runner_defs.get_type() != sajson::TYPE_ARRAY)
		return num_updates;

	uint64_t num_runner_defs = runner_defs.get_length();
	for (uint64_t i = 0; i < num_runner_defs; i++) {
		sajson::value runner_def = runner_defs.get_array_element(i);
		num_updates += parse_runner_definition(state, runner_def, dyn_buf);
	}

	return num_updates;
}

// Parse runner change update.
static auto parse_rc(update_state& state, const sajson::value& rc, dynamic_buffer& dyn_buf)
	-> uint64_t
{
	uint64_t num_updates = 0;

	return num_updates;
}

// Process market change update.
static auto parse_mc(update_state& state, const sajson::value& mc, dynamic_buffer& dyn_buf)
	-> uint64_t
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

	// Now send a market traded volume update.
	sajson::value tv = mc.get_value_of_key(sajson::literal("tv"));
	double traded_vol = 0;
	if (tv.get_type() != sajson::TYPE_NULL)
		traded_vol = tv.get_number_value();
	// If traded volume is reported as 0 this indicates no change.
	if (traded_vol > 0) {
		dyn_buf.add(make_market_traded_vol_update(traded_vol));
		num_updates++;
	}

	// Handle market definition data.
	sajson::value market_def = mc.get_value_of_key(sajson::literal("marketDefinition"));
	num_updates += parse_market_definition(state, market_def, dyn_buf);

	// Handle runner change updates.

	sajson::value rcs = mc.get_value_of_key(sajson::literal("rc"));
	if (rcs.get_type() != sajson::TYPE_ARRAY)
		return num_updates;

	uint64_t num_rcs = rcs.get_length();
	for (uint64_t i = 0; i < num_rcs; i++) {
		sajson::value rc = rcs.get_array_element(i);
		num_updates += parse_rc(state, rc, dyn_buf);
	}

	return num_updates;
}

auto parse_update_stream_json(update_state& state, char* str, uint64_t size,
			      dynamic_buffer& dyn_buf) -> uint64_t
{
	sajson::document doc =
		janus::internal::parse_json(state.filename, str, size, state.line - 1);
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
