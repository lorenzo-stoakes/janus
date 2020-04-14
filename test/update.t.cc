#include "janus.hh"

#include <gtest/gtest.h>
#include <stdexcept>

namespace
{
// Quick and dirty helper function to find first instance of the specified
// update type in a dynamic buffer. Updates dynamic buffer read offset.
static auto find_first_update_of(janus::update_type type, janus::dynamic_buffer& dyn_buf,
				 uint64_t num_updates) -> janus::update*
{
	for (uint64_t i = 0; i < num_updates; i++) {
		janus::update& update = dyn_buf.read<janus::update>();

		if (update.type == type)
			return &update;
	}

	return nullptr;
}

// Test that we throw if the inputted JSON does not contain "op":"mcm" (market change message).
TEST(update_test, op_must_be_mcm)
{
	char json[] =
		R"({"op":"mcm","id":1,"clk":"123","pt":1583884814303,"mc":[{"rc":[{"atb":[[1.01,2495.22]],"id":12635885}],"img":false,"tv":0,"con":false,"id":"1.170020941"}],"status":0})";
	uint64_t size = sizeof(json) - 1;
	// Won't be larger than the JSON buffer.
	janus::dynamic_buffer dyn_buf(size);
	janus::update_state state = {
		.filename = "",
		.line = 1,
	};

	EXPECT_NO_THROW(janus::betfair::parse_update_stream_json(state, json, size, dyn_buf));

	char json2[] =
		R"({"op":"mgm grand","id":1,"clk":"123","pt":1583884814303,"mc":[{"rc":[{"atb":[[1.01,2495.22]],"id":12635885}],"img":false,"tv":0,"con":false,"id":"1.170020941"}],"status":0})";
	uint64_t size2 = sizeof(json2) - 1;

	dyn_buf.reset();
	EXPECT_THROW(janus::betfair::parse_update_stream_json(state, json2, size2, dyn_buf),
		     std::runtime_error);
}

// Test that the line number increments in update state after each successful
// parse operation.
TEST(update_test, line_increments)
{
	char json[] =
		R"({"op":"mcm","id":1,"clk":"123","pt":1583884814303,"mc":[{"rc":[{"atb":[[1.01,2495.22]],"id":12635885}],"img":false,"tv":0,"con":false,"id":"1.170020941"}],"status":0})";
	uint64_t size = sizeof(json) - 1;
	// Won't be larger than the JSON buffer.
	janus::dynamic_buffer dyn_buf(size);
	janus::update_state state = {
		.filename = "",
		.line = 1,
	};

	janus::betfair::parse_update_stream_json(state, json, size, dyn_buf);
	EXPECT_EQ(state.line, 2);

	char json2[] =
		R"({"op":"mcm","id":1,"clk":"123","pt":1583884814303,"mc":[{"rc":[{"atb":[[1.01,2495.22]],"id":12635885}],"img":false,"tv":0,"con":false,"id":"1.170020941"}],"status":0})";
	janus::betfair::parse_update_stream_json(state, json2, size, dyn_buf);
	EXPECT_EQ(state.line, 3);
}

// Test that empty "mc" key results in nothing - in the case of no market change
// updates, we have nothing to do.
TEST(update_test, do_nothing_on_empty_mc)
{
	// Firstly check empty array.

	char json1[] = R"({"op":"mcm","id":1,"clk":"123","pt":1583884814303,"mc":[],"status":0})";
	uint64_t size1 = sizeof(json1) - 1;
	// Won't be larger than the JSON buffer.
	janus::dynamic_buffer dyn_buf(size1);
	janus::update_state state = {
		.filename = "",
		.line = 1,
	};

	EXPECT_EQ(janus::betfair::parse_update_stream_json(state, json1, size1, dyn_buf), 0);
	EXPECT_EQ(state.line, 1);

	// Then a missing "mc" altogether.

	char json2[] = R"({"op":"mcm","id":1,"clk":"123","pt":1583884814303,"status":0})";
	uint64_t size2 = sizeof(json2) - 1;

	EXPECT_EQ(janus::betfair::parse_update_stream_json(state, json2, size2, dyn_buf), 0);
	EXPECT_EQ(state.line, 1);
}

// Test that a change in timestamp correctly results in a timestamp update being sent.
TEST(update_test, send_timestamp_update)
{
	char json1[] =
		R"({"op":"mcm","id":1,"clk":"123","pt":1583884814303,"mc":[{"rc":[{"atb":[[1.01,2495.22]],"id":12635885}],"img":false,"tv":0,"con":false,"id":"1.170020941"}],"status":0})";
	uint64_t size1 = sizeof(json1) - 1;
	janus::dynamic_buffer dyn_buf(10'000'000);
	janus::update_state state = {
		.filename = "",
		.line = 1,
	};

	uint64_t num_updates =
		janus::betfair::parse_update_stream_json(state, json1, size1, dyn_buf);
	EXPECT_GT(num_updates, 0);

	// The first update should be a timestamp update, and since it was not
	// previously set it should be updated.
	auto& update = dyn_buf.read<janus::update>();
	EXPECT_EQ(update.type, janus::update_type::TIMESTAMP);
	EXPECT_EQ(state.timestamp, 1583884814303ULL);

	// Now we have timestamp set, re-processing the JSON should NOT result
	// in a timestamp update.

	dyn_buf.reset();
	char json2[] =
		R"({"op":"mcm","id":1,"clk":"123","pt":1583884814303,"mc":[{"rc":[{"atb":[[1.01,2495.22]],"id":12635885}],"img":false,"tv":0,"con":false,"id":"1.170020941"}],"status":0})";
	uint64_t size2 = sizeof(json2) - 1;

	num_updates = janus::betfair::parse_update_stream_json(state, json2, size2, dyn_buf);

	janus::update* next_update =
		find_first_update_of(janus::update_type::TIMESTAMP, dyn_buf, num_updates);
	EXPECT_EQ(next_update, nullptr);
	EXPECT_EQ(state.timestamp, 1583884814303ULL);
}

// Test that we correctly send a new market ID update when the market ID changes
// and update state accordingly.
TEST(update_test, send_market_id_update)
{
	char json1[] =
		R"({"op":"mcm","id":1,"clk":"123","pt":1583884814303,"mc":[{"rc":[{"atb":[[1.01,2495.22]],"id":12635885}],"img":false,"tv":0,"con":false,"id":"1.170020941"}],"status":0})";
	uint64_t size1 = sizeof(json1) - 1;
	janus::dynamic_buffer dyn_buf(10'000'000);
	janus::update_state state = {
		.filename = "",
		.line = 1,
	};

	uint64_t num_updates =
		janus::betfair::parse_update_stream_json(state, json1, size1, dyn_buf);
	// We should receive at least 1 update as the market ID has changed from
	// unset to 170020941.
	EXPECT_GT(num_updates, 0);
	// State should be updated.
	EXPECT_EQ(state.market_id, 170020941);
	// We should have a market ID update in the dynamic buffer too.
	janus::update* update =
		find_first_update_of(janus::update_type::MARKET_ID, dyn_buf, num_updates);
	ASSERT_NE(update, nullptr);
	uint64_t market_id = janus::get_update_market_id(*update);
	EXPECT_EQ(market_id, 170020941);

	// Now the state has the market ID set, we shouldn't expect re-processing of this JSON
	// should emit a market ID update.

	dyn_buf.reset();
	char json2[] =
		R"({"op":"mcm","id":1,"clk":"123","pt":1583884814303,"mc":[{"rc":[{"atb":[[1.01,2495.22]],"id":12635885}],"img":false,"tv":0,"con":false,"id":"1.170020941"}],"status":0})";
	uint64_t size2 = sizeof(json2) - 1;

	num_updates = janus::betfair::parse_update_stream_json(state, json2, size2, dyn_buf);
	update = find_first_update_of(janus::update_type::MARKET_ID, dyn_buf, num_updates);
	EXPECT_EQ(update, nullptr);
}

// Test that we correctly send a market clear update when "img": true is sent in
// an MC update.
TEST(update_test, send_market_clear_update)
{
	// If "img" is set false, then we don't expect to see a market clear message.

	char json1[] =
		R"({"op":"mcm","id":1,"clk":"123","pt":1583884814303,"mc":[{"rc":[{"atb":[[1.01,2495.22]],"id":12635885}],"img":false,"tv":0,"con":false,"id":"1.170020941"}],"status":0})";
	uint64_t size1 = sizeof(json1) - 1;
	janus::dynamic_buffer dyn_buf(10'000'000);
	janus::update_state state = {
		.filename = "",
		.line = 1,
	};

	uint64_t num_updates =
		janus::betfair::parse_update_stream_json(state, json1, size1, dyn_buf);

	janus::update* update =
		find_first_update_of(janus::update_type::MARKET_CLEAR, dyn_buf, num_updates);
	EXPECT_EQ(update, nullptr);

	// If set true, we should expect to see a market clear message.

	dyn_buf.reset();
	char json2[] =
		R"({"op":"mcm","id":1,"clk":"123","pt":1583884814303,"mc":[{"rc":[{"atb":[[1.01,2495.22]],"id":12635885}],"img":true,"tv":0,"con":false,"id":"1.170020941"}],"status":0})";
	uint64_t size2 = sizeof(json2) - 1;

	num_updates = janus::betfair::parse_update_stream_json(state, json2, size2, dyn_buf);

	// We expect to see the market clear message FIRST or only after a market ID message.

	bool found = false;
	for (uint64_t i = 0; i < num_updates; i++) {
		janus::update& curr_update = dyn_buf.read<janus::update>();
		ASSERT_TRUE(curr_update.type == janus::update_type::TIMESTAMP ||
			    curr_update.type == janus::update_type::MARKET_ID ||
			    curr_update.type == janus::update_type::MARKET_CLEAR);
		if (curr_update.type == janus::update_type::MARKET_CLEAR) {
			found = true;
			break;
		}
	}
	EXPECT_TRUE(found);
}

// Test that we correctly send a market traded volume update when a new traded
// volume update is present in the JSON.
TEST(update_test, send_market_traded_vol_update)
{
	// Firstly we have a traded volume update.

	char json1[] =
		R"({"op":"mcm","id":1,"clk":"123","pt":1583884814303,"mc":[{"rc":[{"atb":[[1.01,2495.22]],"id":12635885}],"img":false,"tv":123.456,"con":false,"id":"1.170020941"}],"status":0})";
	uint64_t size1 = sizeof(json1) - 1;
	janus::dynamic_buffer dyn_buf(10'000'000);
	janus::update_state state = {
		.filename = "",
		.line = 1,
	};

	uint64_t num_updates =
		janus::betfair::parse_update_stream_json(state, json1, size1, dyn_buf);

	janus::update* update =
		find_first_update_of(janus::update_type::MARKET_TRADED_VOL, dyn_buf, num_updates);
	ASSERT_NE(update, nullptr);
	EXPECT_DOUBLE_EQ(get_update_market_traded_vol(*update), 123.456);

	// Now we have "tv": 0, i.e. no change.

	dyn_buf.reset();
	char json2[] =
		R"({"op":"mcm","id":1,"clk":"123","pt":1583884814303,"mc":[{"rc":[{"atb":[[1.01,2495.22]],"id":12635885}],"img":false,"tv":0,"con":false,"id":"1.170020941"}],"status":0})";
	uint64_t size2 = sizeof(json2) - 1;

	// Should not have any market traded volume updates.
	num_updates = janus::betfair::parse_update_stream_json(state, json2, size2, dyn_buf);
	update = find_first_update_of(janus::update_type::MARKET_TRADED_VOL, dyn_buf, num_updates);
	EXPECT_EQ(update, nullptr);

	// And finally a missing "tv" key altogether (probably wouldn't happen
	// in practice but we should handle it).

	dyn_buf.reset();
	char json3[] =
		R"({"op":"mcm","id":1,"clk":"123","pt":1583884814303,"mc":[{"rc":[{"atb":[[1.01,2495.22]],"id":12635885}],"img":false,"con":false,"id":"1.170020941"}],"status":0})";
	uint64_t size3 = sizeof(json3) - 1;

	// Should not have any market traded volume updates.
	num_updates = janus::betfair::parse_update_stream_json(state, json3, size3, dyn_buf);
	update = find_first_update_of(janus::update_type::MARKET_TRADED_VOL, dyn_buf, num_updates);
	EXPECT_EQ(update, nullptr);
}
} // namespace
