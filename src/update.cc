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
static auto get_error_prefix(const update_state& state) -> std::string
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

// Remove all runner ID updates from the end of the dynamic buffer. This is used
// to, immediately prior to adding a runner ID, remove any existing defunct
// runner ID updates (switching from 1 runner ID to another back-to-back
// achieves nothing).
static auto strip_runner_ids(dynamic_buffer& dyn_buf) -> int64_t
{
	if (dyn_buf.size() < sizeof(update))
		return 0;

	// Rewind to the last update.
	update* ptr = reinterpret_cast<update*>(dyn_buf.data());
	uint64_t num_updates = dyn_buf.size() / sizeof(update);

	int64_t offset = 0;
	for (int64_t i = num_updates - 1; i >= 0; i--) {
		update& update = ptr[i];

		if (update.type != update_type::RUNNER_ID)
			break;

		offset++;
	}

	// Remove discovered runner IDs.
	dyn_buf.rewind(offset * sizeof(update));
	return -offset;
}

// Emit runner ID update if needed.
static auto send_runner_id(update_state& state, dynamic_buffer& dyn_buf, uint64_t runner_id)
	-> int64_t
{
	if (state.runner_id == runner_id)
		return 0;

	int64_t num_updates = strip_runner_ids(dyn_buf);

	dyn_buf.add(make_runner_id_update(runner_id));
	state.runner_id = runner_id;
	num_updates++;

	return num_updates;
}

// Parse runner definition in order to extract any non-runner or runner BSP data.
static auto parse_runner_definition(update_state& state, const sajson::value& runner_def,
				    dynamic_buffer& dyn_buf) -> int64_t
{
	// We may duplicate data sent from here, again the reciever of this data
	// should be able to handle it being sent more than once for a runner.

	int64_t num_updates = 0;

	// Nothing to do.
	if (runner_def.get_type() != sajson::TYPE_OBJECT)
		return 0;

	sajson::value id = runner_def.get_value_of_key(sajson::literal("id"));
	uint64_t runner_id = id.get_integer_value();

	sajson::value bsp = runner_def.get_value_of_key(sajson::literal("bsp"));
	// Sometimes betfair sends BSP of "NaN" (!)
	if (bsp.get_type() != sajson::TYPE_NULL && bsp.get_type() != sajson::TYPE_STRING) {
		num_updates += send_runner_id(state, dyn_buf, runner_id);

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
		double adj_factor_val = 0;
		if (adj_factor.get_type() != sajson::TYPE_NULL)
			adj_factor_val = adj_factor.get_number_value();

		num_updates += send_runner_id(state, dyn_buf, runner_id);
		dyn_buf.add(make_runner_removal_update(adj_factor_val));
		num_updates++;
	} else if (size == sizeof("WINNER") - 1 &&
		   ::strncmp(status_str, "WINNER", sizeof("WINNER") - 1) == 0) {
		num_updates += send_runner_id(state, dyn_buf, runner_id);
		dyn_buf.add(make_runner_won_update());
		num_updates++;
	}

	return num_updates;
}

// Parse market definition node and generate updates as required.
static auto parse_market_definition(update_state& state, const sajson::value& market_def,
				    dynamic_buffer& dyn_buf) -> int64_t
{
	// Nothing to do.
	if (market_def.get_type() != sajson::TYPE_OBJECT)
		return 0;

	int64_t num_updates = 0;

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

// Parse matched volume [ price, vol ] pair.
static auto parse_trd(const update_state& state, const sajson::value& trd, dynamic_buffer& dyn_buf)
	-> int64_t
{
	const price_range* range = state.range;

	// We assume the data is in the correct format.
	decimal7 price = trd.get_array_element(0).get_decimal_value();
	// Sometimes after a runner removal the matched volume prices will be
	// all over the place as they have been scaled by the adjustment factor.
	// Really we should assign to the surrounding price levels weighted by
	// how close they are to them, however for now we simply round down.
	// TODO(lorenzo): Review.
	uint32_t price_index = range->pricex100_to_nearest_index(price.mult100());
	double vol = trd.get_array_element(1).get_number_value();

	dyn_buf.add(make_runner_matched_update(price_index, vol));
	return 1;
}

// Parse ATL [ price, vol ] pair.
static auto parse_atl(const update_state& state, const sajson::value& atl, dynamic_buffer& dyn_buf)
	-> int64_t
{
	const price_range* range = state.range;

	// We assume the data is in the correct format.
	decimal7 price = atl.get_array_element(0).get_decimal_value();
	uint64_t price_index = range->pricex100_to_index(price.mult100());
	if (price_index == INVALID_PRICE_INDEX)
		throw std::runtime_error(get_error_prefix(state) + std::to_string(state.runner_id) +
					 ": ATL of " + std::to_string(price.to_double()) +
					 " is invalid");
	double vol = atl.get_array_element(1).get_number_value();

	dyn_buf.add(make_runner_unmatched_atl_update(price_index, vol));
	return 1;
}

// Parse ATB [ price, vol ] pair.
static auto parse_atb(const update_state& state, const sajson::value& atb, dynamic_buffer& dyn_buf)
	-> int64_t
{
	const price_range* range = state.range;

	// We assume the data is in the correct format.
	decimal7 price = atb.get_array_element(0).get_decimal_value();
	uint64_t price_index = range->pricex100_to_index(price.mult100());
	if (price_index == INVALID_PRICE_INDEX)
		throw std::runtime_error(get_error_prefix(state) + std::to_string(state.runner_id) +
					 ": ATB of " + std::to_string(price.to_double()) +
					 " is invalid");
	double vol = atb.get_array_element(1).get_number_value();

	dyn_buf.add(make_runner_unmatched_atb_update(price_index, vol));
	return 1;
}

// Reorder ATL/ATB updates in the dynamic buffer such that updates which clear
// ATL/ATB unmatched volume come FIRST. This is necessary as betfair sends these
// in an arbitrary order and updates could end up causing invalid state if
// clears are not processed first.
//
// Also if ATL/ATB updates indicate an IMPOSSIBLE situation i.e. max ATB > min
// ATL, we remove ATL/ATB updates and send a runner clear unmatched update.
//
// Returns number of updates SUBTRACTED from buffer.
static auto workaround_atl_atb(uint64_t delta_updates, dynamic_buffer& dyn_buf) -> int64_t
{
	// Access last element in buffer.
	uint8_t* raw = reinterpret_cast<uint8_t*>(dyn_buf.data());
	raw += dyn_buf.size();
	// Then rewind to the start of the ATL/ATB updates.
	auto* ptr = reinterpret_cast<update*>(raw) - delta_updates;
	auto* write_ptr = ptr;

	bool seen_min_atl = false;
	bool seen_max_atb = false;

	uint64_t min_atl_price_index = NUM_PRICES - 1;
	uint64_t max_atb_price_index = 0;

	for (uint64_t i = 0; i < delta_updates; i++) {
		update update = ptr[i];

		double vol;
		if (update.type == update_type::RUNNER_UNMATCHED_ATL) {
			uint64_t price_index;
			std::tie(price_index, vol) = get_update_runner_unmatched_atl(update);

			if (price_index < min_atl_price_index && vol != 0) {
				seen_min_atl = true;
				min_atl_price_index = price_index;
			}

		} else if (update.type == update_type::RUNNER_UNMATCHED_ATB) {
			uint64_t price_index;
			std::tie(price_index, vol) = get_update_runner_unmatched_atb(update);

			if (price_index > max_atb_price_index && vol != 0) {
				seen_max_atb = true;
				max_atb_price_index = price_index;
			}
		} else {
			throw std::runtime_error(std::string("Unexpected update type ") +
						 update_type_str(update.type) +
						 " on reorder ATL/ATB");
		}

		if (vol == 0) {
			// If we have a clearing update, i.e. volume = 0, swap these
			// update with the update at the current write offset.
			ptr[i] = *write_ptr;
			*write_ptr++ = update;
		}
	}

	if (!seen_min_atl || !seen_max_atb || max_atb_price_index < min_atl_price_index)
		return 0;

	// OK we've received an impossible update, this data is clearly
	// corrupted so simply clear the entire runner unmatched ladder. Looking
	// at real data I see no more than 0.01% markets with unmatched
	// corruption issues not handled with the above workaround so this is
	// hardly a common scenario.

	// Undo ATL/ATB updates other than last one which we will overwrite.
	dyn_buf.rewind((delta_updates - 1) * sizeof(update));
	update& update = ptr[0];
	update = make_runner_clear_unmatched();

	return delta_updates - 1;
}

// Parse runner change update.
static auto parse_rc(update_state& state, const sajson::value& rc, dynamic_buffer& dyn_buf)
	-> int64_t
{
	int64_t num_updates = 0;

	const price_range* range = state.range;

	// Send runner ID if needed.

	sajson::value id = rc.get_value_of_key(sajson::literal("id"));
	uint64_t runner_id = id.get_integer_value();
	num_updates += send_runner_id(state, dyn_buf, runner_id);

	// Traded volume.

	sajson::value tv = rc.get_value_of_key(sajson::literal("tv"));
	if (tv.get_type() != sajson::TYPE_NULL) {
		double tv_vol = tv.get_number_value();
		// TV of 0 means no change.
		if (tv_vol > 0) {
			dyn_buf.add(make_runner_traded_vol_update(tv_vol));
			num_updates++;
		}
	}

	// LTP.

	sajson::value ltp = rc.get_value_of_key(sajson::literal("ltp"));
	if (ltp.get_type() != sajson::TYPE_NULL) {
		decimal7 ltp_val = ltp.get_decimal_value();

		// Sometimes after a runner removal the LTP will be invalid as
		// it will have been scaled by the adjustment factor. We work
		// around this by rounding down.
		// TODO(lorenzo): Review.
		uint32_t price_index = range->pricex100_to_nearest_index(ltp_val.mult100());

		dyn_buf.add(make_runner_ltp_update(price_index));
		num_updates++;
	}

	// Matched volume.

	sajson::value trds = rc.get_value_of_key(sajson::literal("trd"));
	uint64_t num_trds = 0;
	if (trds.get_type() == sajson::TYPE_ARRAY)
		num_trds = trds.get_length();
	for (uint64_t i = 0; i < num_trds; i++) {
		sajson::value trd = trds.get_array_element(i);
		num_updates += parse_trd(state, trd, dyn_buf);
	}

	uint64_t before_atl_atb = num_updates;

	// ATL.

	sajson::value atls = rc.get_value_of_key(sajson::literal("atl"));
	uint64_t num_atls = 0;
	if (atls.get_type() == sajson::TYPE_ARRAY)
		num_atls = atls.get_length();
	for (uint64_t i = 0; i < num_atls; i++) {
		sajson::value atl = atls.get_array_element(i);
		num_updates += parse_atl(state, atl, dyn_buf);
	}
	bool have_atls = num_atls > 0;

	// ATB.

	sajson::value atbs = rc.get_value_of_key(sajson::literal("atb"));
	uint64_t num_atbs = 0;
	if (atbs.get_type() == sajson::TYPE_ARRAY)
		num_atbs = atbs.get_length();
	for (uint64_t i = 0; i < num_atbs; i++) {
		sajson::value atb = atbs.get_array_element(i);
		num_updates += parse_atb(state, atb, dyn_buf);
	}
	bool have_atbs = num_atbs > 0;

	// If we have ATL and ATB updates we need to re-order them such that
	// updates that clear unmatched volume come first.
	// This is because betfair will send these clearing values in any order
	// they please, which can result in invalid update state when applied.
	//
	// Additionally, we need to be able to clear runner unmatched state when
	// corrupted unmatched data is sent (thanks again betfair).
	if (have_atls && have_atbs)
		num_updates -= workaround_atl_atb(num_updates - before_atl_atb, dyn_buf);

	return num_updates;
}

// Process market change update.
static auto parse_mc(update_state& state, const sajson::value& mc, dynamic_buffer& dyn_buf)
	-> int64_t
{
	sajson::value id = mc.get_value_of_key(sajson::literal("id"));
	uint64_t market_id =
		janus::internal::parse_market_id(id.as_cstring(), id.get_string_length());

	int64_t num_updates = 0;

	// If the market ID has changed from the last market ID update sent, we
	// need to send + update.
	if (state.market_id != market_id) {
		dyn_buf.add(make_market_id_update(market_id));
		num_updates++;
		state.market_id = market_id;
		state.runner_id = 0;
	}

	// "img": true indicates this is a snapshot update not a delta, so we
	// need to clear down market data cache.
	sajson::value img = mc.get_value_of_key(sajson::literal("img"));
	if (img.get_type() == sajson::TYPE_TRUE) {
		dyn_buf.add(make_market_clear_update());
		num_updates++;
		state.runner_id = 0;
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

	int64_t num_updates = 0;

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
	return static_cast<uint64_t>(num_updates);
}
} // namespace janus::betfair
