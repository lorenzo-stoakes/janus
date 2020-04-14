#include "janus.hh"

#include <cmath>
#include <gtest/gtest.h>
#include <stdexcept>
#include <vector>

namespace
{
const static janus::betfair::price_range range;

// Quick and dirty function to round a number to 2 decimal places.
static double round_2dp(double n)
{
	uint64_t nx100 = ::llround(n * 100.);
	return static_cast<double>(nx100) / 100;
}

// Quick and dirty helper function to find first instance of the specified
// update type in a dynamic buffer. Updates dynamic buffer read offset.
static auto find_first_update_of(janus::update_type type, janus::dynamic_buffer& dyn_buf,
				 uint64_t num_updates, uint64_t* runner_id_ptr = nullptr)
	-> janus::update*
{
	for (uint64_t i = 0; i < num_updates; i++) {
		janus::update& update = dyn_buf.read<janus::update>();

		if (runner_id_ptr != nullptr && update.type == janus::update_type::RUNNER_ID)
			*runner_id_ptr = janus::get_update_runner_id(update);

		if (update.type == type)
			return &update;
	}

	return nullptr;
}

// Quick and dirty helper function to find all instances of the specified update
// type in a dynamic buffer with last reported runner ID.
static auto find_all_updates_of(janus::update_type type, janus::dynamic_buffer& dyn_buf,
				uint64_t num_updates)
	-> std::vector<std::pair<uint64_t, janus::update>>
{
	std::vector<std::pair<uint64_t, janus::update>> ret;

	uint64_t runner_id = 0;
	for (uint64_t i = 0; i < num_updates; i++) {
		janus::update& update = dyn_buf.read<janus::update>();

		if (update.type == janus::update_type::RUNNER_ID)
			runner_id = janus::get_update_runner_id(update);

		if (update.type != type)
			continue;

		ret.emplace_back(std::make_pair(runner_id, update));
	}

	return ret;
}

// Test that we throw if the inputted JSON does not contain "op":"mcm" (market change message).
TEST(update_test, op_must_be_mcm)
{
	char json[] =
		R"({"op":"mcm","id":1,"clk":"123","pt":1583884814303,"mc":[{"rc":[{"atb":[[1.01,2495.22]],"id":12635885}],"img":false,"tv":0,"con":false,"id":"1.170020941"}],"status":0})";
	uint64_t size = sizeof(json) - 1;
	// Won't be larger than the JSON buffer.
	janus::dynamic_buffer dyn_buf(size);
	janus::betfair::update_state state = {
		.range = &range,
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
	janus::betfair::update_state state = {
		.range = &range,
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
	janus::betfair::update_state state = {
		.range = &range,
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
	janus::betfair::update_state state = {
		.range = &range,
		.filename = "",
		.line = 1,
	};

	uint64_t num_updates =
		janus::betfair::parse_update_stream_json(state, json1, size1, dyn_buf);
	EXPECT_GT(num_updates, 0);
	EXPECT_EQ(dyn_buf.size(), sizeof(janus::update) * num_updates);

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
	EXPECT_EQ(dyn_buf.size(), sizeof(janus::update) * num_updates);

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
	janus::betfair::update_state state = {
		.range = &range,
		.filename = "",
		.line = 1,
	};

	uint64_t num_updates =
		janus::betfair::parse_update_stream_json(state, json1, size1, dyn_buf);
	// We should receive at least 1 update as the market ID has changed from
	// unset to 170020941.
	EXPECT_GT(num_updates, 0);
	EXPECT_EQ(dyn_buf.size(), sizeof(janus::update) * num_updates);
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
	EXPECT_EQ(dyn_buf.size(), sizeof(janus::update) * num_updates);
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
	janus::betfair::update_state state = {
		.range = &range,
		.filename = "",
		.line = 1,
	};

	uint64_t num_updates =
		janus::betfair::parse_update_stream_json(state, json1, size1, dyn_buf);
	EXPECT_EQ(dyn_buf.size(), sizeof(janus::update) * num_updates);

	janus::update* update =
		find_first_update_of(janus::update_type::MARKET_CLEAR, dyn_buf, num_updates);
	EXPECT_EQ(update, nullptr);

	// If set true, we should expect to see a market clear message.

	dyn_buf.reset();
	char json2[] =
		R"({"op":"mcm","id":1,"clk":"123","pt":1583884814303,"mc":[{"rc":[{"atb":[[1.01,2495.22]],"id":12635885}],"img":true,"tv":0,"con":false,"id":"1.170020941"}],"status":0})";
	uint64_t size2 = sizeof(json2) - 1;

	num_updates = janus::betfair::parse_update_stream_json(state, json2, size2, dyn_buf);
	EXPECT_EQ(dyn_buf.size(), sizeof(janus::update) * num_updates);

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
	janus::betfair::update_state state = {
		.range = &range,
		.filename = "",
		.line = 1,
	};

	uint64_t num_updates =
		janus::betfair::parse_update_stream_json(state, json1, size1, dyn_buf);
	EXPECT_EQ(dyn_buf.size(), sizeof(janus::update) * num_updates);

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
	EXPECT_EQ(dyn_buf.size(), sizeof(janus::update) * num_updates);
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
	EXPECT_EQ(dyn_buf.size(), sizeof(janus::update) * num_updates);
	update = find_first_update_of(janus::update_type::MARKET_TRADED_VOL, dyn_buf, num_updates);
	EXPECT_EQ(update, nullptr);
}

// Test that we correctly send a market open, close, supsend or inplay update
// when the update JSON indicates the market status accordingly.
TEST(update_test, send_market_status_update)
{
	// Market OPEN.

	char json1[] =
		R"({"op":"mcm","id":1,"clk":"1234","pt":1583932968339,"mc":[{"rc":[],"img":false,"tv":0,"con":false,"marketDefinition":{"venue":"Lingfield","raceType":"Flat","timezone":"Europe/London","regulators":["MR_NJ","MR_INT"],"marketType":"WIN","marketBaseRate":5,"numberOfWinners":1,"countryCode":"GB","inPlay":true,"betDelay":1,"bspMarket":true,"bettingType":"ODDS","numberOfActiveRunners":10,"eventId":"29746086","crossMatching":true,"turnInPlayEnabled":true,"priceLadderDefinition":{"type":"CLASSIC"},"suspendTime":"2020-03-11T13:20:00Z","discountAllowed":true,"persistenceEnabled":true,"runners":[{"sortPriority":1,"id":13233309,"adjustmentFactor":26.667,"bsp":4.7,"status":"ACTIVE"},{"sortPriority":2,"id":11622845,"adjustmentFactor":17.554,"bsp":4.1977740716207315,"status":"ACTIVE"},{"sortPriority":3,"id":15062473,"adjustmentFactor":13.514,"bsp":8.8,"status":"ACTIVE"},{"sortPriority":4,"id":17247906,"adjustmentFactor":10.417,"bsp":11.716258968405892,"status":"ACTIVE"},{"sortPriority":5,"id":12635885,"adjustmentFactor":9.615,"bsp":8.896559078420763,"status":"ACTIVE"},{"sortPriority":6,"id":10220888,"adjustmentFactor":6.667,"bsp":15.5,"status":"ACTIVE"},{"sortPriority":7,"id":24270884,"adjustmentFactor":6.25,"bsp":17.006234472860235,"status":"ACTIVE"},{"sortPriority":8,"id":18889965,"adjustmentFactor":4.348,"bsp":13.5,"status":"ACTIVE"},{"sortPriority":9,"id":22109331,"adjustmentFactor":3.704,"bsp":29.661771640139044,"status":"ACTIVE"},{"sortPriority":10,"id":10322409,"adjustmentFactor":1.266,"bsp":100,"status":"ACTIVE"},{"sortPriority":11,"removalDate":"2020-03-10T18:58:19Z","id":12481874,"adjustmentFactor":0.2,"status":"REMOVED"},{"sortPriority":12,"removalDate":"2020-03-11T07:57:17Z","id":13229196,"adjustmentFactor":2.632,"status":"REMOVED"}],"version":3224121565,"eventTypeId":"7","complete":true,"openDate":"2020-03-11T13:20:00Z","marketTime":"2020-03-11T13:20:00Z","bspReconciled":true,"status":"OPEN"},"id":"1.170020941"}],"status":0})";
	uint64_t size1 = sizeof(json1) - 1;
	janus::dynamic_buffer dyn_buf(10'000'000);
	janus::betfair::update_state state = {
		.range = &range,
		.filename = "",
		.line = 1,
	};

	uint64_t num_updates =
		janus::betfair::parse_update_stream_json(state, json1, size1, dyn_buf);
	EXPECT_EQ(dyn_buf.size(), sizeof(janus::update) * num_updates);
	janus::update* update =
		find_first_update_of(janus::update_type::MARKET_OPEN, dyn_buf, num_updates);
	ASSERT_NE(update, nullptr);

	// Market CLOSED.

	dyn_buf.reset();
	char json2[] =
		R"({"op":"mcm","id":1,"clk":"1234","pt":1583932968339,"mc":[{"rc":[],"img":false,"tv":0,"con":false,"marketDefinition":{"venue":"Lingfield","raceType":"Flat","timezone":"Europe/London","regulators":["MR_NJ","MR_INT"],"marketType":"WIN","marketBaseRate":5,"numberOfWinners":1,"countryCode":"GB","inPlay":true,"betDelay":1,"bspMarket":true,"bettingType":"ODDS","numberOfActiveRunners":10,"eventId":"29746086","crossMatching":true,"turnInPlayEnabled":true,"priceLadderDefinition":{"type":"CLASSIC"},"suspendTime":"2020-03-11T13:20:00Z","discountAllowed":true,"persistenceEnabled":true,"runners":[{"sortPriority":1,"id":13233309,"adjustmentFactor":26.667,"bsp":4.7,"status":"ACTIVE"},{"sortPriority":2,"id":11622845,"adjustmentFactor":17.554,"bsp":4.1977740716207315,"status":"ACTIVE"},{"sortPriority":3,"id":15062473,"adjustmentFactor":13.514,"bsp":8.8,"status":"ACTIVE"},{"sortPriority":4,"id":17247906,"adjustmentFactor":10.417,"bsp":11.716258968405892,"status":"ACTIVE"},{"sortPriority":5,"id":12635885,"adjustmentFactor":9.615,"bsp":8.896559078420763,"status":"ACTIVE"},{"sortPriority":6,"id":10220888,"adjustmentFactor":6.667,"bsp":15.5,"status":"ACTIVE"},{"sortPriority":7,"id":24270884,"adjustmentFactor":6.25,"bsp":17.006234472860235,"status":"ACTIVE"},{"sortPriority":8,"id":18889965,"adjustmentFactor":4.348,"bsp":13.5,"status":"ACTIVE"},{"sortPriority":9,"id":22109331,"adjustmentFactor":3.704,"bsp":29.661771640139044,"status":"ACTIVE"},{"sortPriority":10,"id":10322409,"adjustmentFactor":1.266,"bsp":100,"status":"ACTIVE"},{"sortPriority":11,"removalDate":"2020-03-10T18:58:19Z","id":12481874,"adjustmentFactor":0.2,"status":"REMOVED"},{"sortPriority":12,"removalDate":"2020-03-11T07:57:17Z","id":13229196,"adjustmentFactor":2.632,"status":"REMOVED"}],"version":3224121565,"eventTypeId":"7","complete":true,"openDate":"2020-03-11T13:20:00Z","marketTime":"2020-03-11T13:20:00Z","bspReconciled":true,"status":"CLOSED"},"id":"1.170020941"}],"status":0})";
	uint64_t size2 = sizeof(json2) - 1;

	num_updates = janus::betfair::parse_update_stream_json(state, json2, size2, dyn_buf);
	EXPECT_EQ(dyn_buf.size(), sizeof(janus::update) * num_updates);
	update = find_first_update_of(janus::update_type::MARKET_CLOSE, dyn_buf, num_updates);
	ASSERT_NE(update, nullptr);

	// Market SUSPENDED.

	dyn_buf.reset();
	char json3[] =
		R"({"op":"mcm","id":1,"clk":"1234","pt":1583932968339,"mc":[{"rc":[],"img":false,"tv":0,"con":false,"marketDefinition":{"venue":"Lingfield","raceType":"Flat","timezone":"Europe/London","regulators":["MR_NJ","MR_INT"],"marketType":"WIN","marketBaseRate":5,"numberOfWinners":1,"countryCode":"GB","inPlay":true,"betDelay":1,"bspMarket":true,"bettingType":"ODDS","numberOfActiveRunners":10,"eventId":"29746086","crossMatching":true,"turnInPlayEnabled":true,"priceLadderDefinition":{"type":"CLASSIC"},"suspendTime":"2020-03-11T13:20:00Z","discountAllowed":true,"persistenceEnabled":true,"runners":[{"sortPriority":1,"id":13233309,"adjustmentFactor":26.667,"bsp":4.7,"status":"ACTIVE"},{"sortPriority":2,"id":11622845,"adjustmentFactor":17.554,"bsp":4.1977740716207315,"status":"ACTIVE"},{"sortPriority":3,"id":15062473,"adjustmentFactor":13.514,"bsp":8.8,"status":"ACTIVE"},{"sortPriority":4,"id":17247906,"adjustmentFactor":10.417,"bsp":11.716258968405892,"status":"ACTIVE"},{"sortPriority":5,"id":12635885,"adjustmentFactor":9.615,"bsp":8.896559078420763,"status":"ACTIVE"},{"sortPriority":6,"id":10220888,"adjustmentFactor":6.667,"bsp":15.5,"status":"ACTIVE"},{"sortPriority":7,"id":24270884,"adjustmentFactor":6.25,"bsp":17.006234472860235,"status":"ACTIVE"},{"sortPriority":8,"id":18889965,"adjustmentFactor":4.348,"bsp":13.5,"status":"ACTIVE"},{"sortPriority":9,"id":22109331,"adjustmentFactor":3.704,"bsp":29.661771640139044,"status":"ACTIVE"},{"sortPriority":10,"id":10322409,"adjustmentFactor":1.266,"bsp":100,"status":"ACTIVE"},{"sortPriority":11,"removalDate":"2020-03-10T18:58:19Z","id":12481874,"adjustmentFactor":0.2,"status":"REMOVED"},{"sortPriority":12,"removalDate":"2020-03-11T07:57:17Z","id":13229196,"adjustmentFactor":2.632,"status":"REMOVED"}],"version":3224121565,"eventTypeId":"7","complete":true,"openDate":"2020-03-11T13:20:00Z","marketTime":"2020-03-11T13:20:00Z","bspReconciled":true,"status":"SUSPENDED"},"id":"1.170020941"}],"status":0})";
	uint64_t size3 = sizeof(json3) - 1;

	num_updates = janus::betfair::parse_update_stream_json(state, json3, size3, dyn_buf);
	EXPECT_EQ(dyn_buf.size(), sizeof(janus::update) * num_updates);
	update = find_first_update_of(janus::update_type::MARKET_SUSPEND, dyn_buf, num_updates);
	ASSERT_NE(update, nullptr);

	// Market INPLAY.

	dyn_buf.reset();
	char json4[] =
		R"({"op":"mcm","id":1,"clk":"1234","pt":1583932968339,"mc":[{"rc":[],"img":false,"tv":0,"con":false,"marketDefinition":{"venue":"Lingfield","raceType":"Flat","timezone":"Europe/London","regulators":["MR_NJ","MR_INT"],"marketType":"WIN","marketBaseRate":5,"numberOfWinners":1,"countryCode":"GB","inPlay":true,"betDelay":1,"bspMarket":true,"bettingType":"ODDS","numberOfActiveRunners":10,"eventId":"29746086","crossMatching":true,"turnInPlayEnabled":true,"priceLadderDefinition":{"type":"CLASSIC"},"suspendTime":"2020-03-11T13:20:00Z","discountAllowed":true,"persistenceEnabled":true,"runners":[{"sortPriority":1,"id":13233309,"adjustmentFactor":26.667,"bsp":4.7,"status":"ACTIVE"},{"sortPriority":2,"id":11622845,"adjustmentFactor":17.554,"bsp":4.1977740716207315,"status":"ACTIVE"},{"sortPriority":3,"id":15062473,"adjustmentFactor":13.514,"bsp":8.8,"status":"ACTIVE"},{"sortPriority":4,"id":17247906,"adjustmentFactor":10.417,"bsp":11.716258968405892,"status":"ACTIVE"},{"sortPriority":5,"id":12635885,"adjustmentFactor":9.615,"bsp":8.896559078420763,"status":"ACTIVE"},{"sortPriority":6,"id":10220888,"adjustmentFactor":6.667,"bsp":15.5,"status":"ACTIVE"},{"sortPriority":7,"id":24270884,"adjustmentFactor":6.25,"bsp":17.006234472860235,"status":"ACTIVE"},{"sortPriority":8,"id":18889965,"adjustmentFactor":4.348,"bsp":13.5,"status":"ACTIVE"},{"sortPriority":9,"id":22109331,"adjustmentFactor":3.704,"bsp":29.661771640139044,"status":"ACTIVE"},{"sortPriority":10,"id":10322409,"adjustmentFactor":1.266,"bsp":100,"status":"ACTIVE"},{"sortPriority":11,"removalDate":"2020-03-10T18:58:19Z","id":12481874,"adjustmentFactor":0.2,"status":"REMOVED"},{"sortPriority":12,"removalDate":"2020-03-11T07:57:17Z","id":13229196,"adjustmentFactor":2.632,"status":"REMOVED"}],"version":3224121565,"eventTypeId":"7","complete":true,"openDate":"2020-03-11T13:20:00Z","marketTime":"2020-03-11T13:20:00Z","bspReconciled":true,"status":"OPEN"},"id":"1.170020941"}],"status":0})";
	uint64_t size4 = sizeof(json4) - 1;

	num_updates = janus::betfair::parse_update_stream_json(state, json4, size4, dyn_buf);
	EXPECT_EQ(dyn_buf.size(), sizeof(janus::update) * num_updates);
	update = find_first_update_of(janus::update_type::MARKET_INPLAY, dyn_buf, num_updates);
	ASSERT_NE(update, nullptr);
}

// Test that runner SP, removal and won updates are sent correctly.
TEST(update_test, send_runner_def_update)
{
	char json1[] =
		R"({"op":"mcm","id":1,"clk":"1234","pt":1583932968339,"mc":[{"rc":[],"img":false,"tv":0,"con":false,"marketDefinition":{"venue":"Lingfield","raceType":"Flat","timezone":"Europe/London","regulators":["MR_NJ","MR_INT"],"marketType":"WIN","marketBaseRate":5,"numberOfWinners":1,"countryCode":"GB","inPlay":true,"betDelay":1,"bspMarket":true,"bettingType":"ODDS","numberOfActiveRunners":10,"eventId":"29746086","crossMatching":true,"turnInPlayEnabled":true,"priceLadderDefinition":{"type":"CLASSIC"},"suspendTime":"2020-03-11T13:20:00Z","discountAllowed":true,"persistenceEnabled":true,"runners":[{"sortPriority":1,"id":13233309,"adjustmentFactor":26.667,"bsp":4.7,"status":"WINNER"},{"sortPriority":2,"id":11622845,"adjustmentFactor":17.554,"bsp":4.1977740716207315,"status":"ACTIVE"},{"sortPriority":3,"id":15062473,"adjustmentFactor":13.514,"bsp":8.8,"status":"ACTIVE"},{"sortPriority":4,"id":17247906,"adjustmentFactor":10.417,"bsp":11.716258968405892,"status":"ACTIVE"},{"sortPriority":5,"id":12635885,"adjustmentFactor":9.615,"bsp":8.896559078420763,"status":"ACTIVE"},{"sortPriority":6,"id":10220888,"adjustmentFactor":6.667,"bsp":15.5,"status":"ACTIVE"},{"sortPriority":7,"id":24270884,"adjustmentFactor":6.25,"bsp":17.006234472860235,"status":"ACTIVE"},{"sortPriority":8,"id":18889965,"adjustmentFactor":4.348,"bsp":13.5,"status":"ACTIVE"},{"sortPriority":9,"id":22109331,"adjustmentFactor":3.704,"bsp":29.661771640139044,"status":"ACTIVE"},{"sortPriority":10,"id":10322409,"adjustmentFactor":1.266,"bsp":100,"status":"ACTIVE"},{"sortPriority":11,"removalDate":"2020-03-10T18:58:19Z","id":12481874,"adjustmentFactor":0.2,"status":"REMOVED"},{"sortPriority":12,"removalDate":"2020-03-11T07:57:17Z","id":13229196,"adjustmentFactor":2.632,"status":"REMOVED"}],"version":3224121565,"eventTypeId":"7","complete":true,"openDate":"2020-03-11T13:20:00Z","marketTime":"2020-03-11T13:20:00Z","bspReconciled":true,"status":"OPEN"},"id":"1.170020941"}],"status":0})";
	uint64_t size1 = sizeof(json1) - 1;
	janus::dynamic_buffer dyn_buf(10'000'000);
	janus::betfair::update_state state = {
		.range = &range,
		.filename = "",
		.line = 1,
	};

	// Runner SP.

	// 10 of 12 runners have an SP, the others are removed.
	uint64_t num_updates =
		janus::betfair::parse_update_stream_json(state, json1, size1, dyn_buf);
	EXPECT_EQ(dyn_buf.size(), sizeof(janus::update) * num_updates);
	auto pairs = find_all_updates_of(janus::update_type::RUNNER_SP, dyn_buf, num_updates);
	EXPECT_EQ(pairs.size(), 10);

	EXPECT_EQ(pairs[0].first, 13233309);
	EXPECT_DOUBLE_EQ(round_2dp(janus::get_update_runner_sp(pairs[0].second)), 4.7);

	EXPECT_EQ(pairs[1].first, 11622845);
	EXPECT_DOUBLE_EQ(round_2dp(janus::get_update_runner_sp(pairs[1].second)), 4.20);

	EXPECT_EQ(pairs[2].first, 15062473);
	EXPECT_DOUBLE_EQ(round_2dp(janus::get_update_runner_sp(pairs[2].second)), 8.8);

	EXPECT_EQ(pairs[3].first, 17247906);
	EXPECT_DOUBLE_EQ(round_2dp(janus::get_update_runner_sp(pairs[3].second)), 11.72);

	EXPECT_EQ(pairs[4].first, 12635885);
	EXPECT_DOUBLE_EQ(round_2dp(janus::get_update_runner_sp(pairs[4].second)), 8.9);

	EXPECT_EQ(pairs[5].first, 10220888);
	EXPECT_DOUBLE_EQ(round_2dp(janus::get_update_runner_sp(pairs[5].second)), 15.5);

	EXPECT_EQ(pairs[6].first, 24270884);
	EXPECT_DOUBLE_EQ(round_2dp(janus::get_update_runner_sp(pairs[6].second)), 17.01);

	EXPECT_EQ(pairs[7].first, 18889965);
	EXPECT_DOUBLE_EQ(round_2dp(janus::get_update_runner_sp(pairs[7].second)), 13.5);

	EXPECT_EQ(pairs[8].first, 22109331);
	EXPECT_DOUBLE_EQ(round_2dp(janus::get_update_runner_sp(pairs[8].second)), 29.66);

	EXPECT_EQ(pairs[9].first, 10322409);
	EXPECT_DOUBLE_EQ(round_2dp(janus::get_update_runner_sp(pairs[9].second)), 100);

	// Runner removals.

	dyn_buf.reset_read();
	pairs = find_all_updates_of(janus::update_type::RUNNER_REMOVAL, dyn_buf, num_updates);
	EXPECT_EQ(pairs.size(), 2);

	EXPECT_EQ(pairs[0].first, 12481874);
	EXPECT_DOUBLE_EQ(round_2dp(get_update_runner_adj_factor(pairs[0].second)), 0.2);

	EXPECT_EQ(pairs[1].first, 13229196);
	EXPECT_DOUBLE_EQ(round_2dp(get_update_runner_adj_factor(pairs[1].second)), 2.63);

	// Runner won.

	dyn_buf.reset_read();
	pairs = find_all_updates_of(janus::update_type::RUNNER_WON, dyn_buf, num_updates);
	EXPECT_EQ(pairs.size(), 1);
	EXPECT_EQ(pairs[0].first, 13233309);
}

// Test that we can receive runner data updates - ATL, ATB, LTP, TV, matched volume.
TEST(update_test, send_runner_update)
{
	// No LTP, no TV, no matched volume.

	char json1[] =
		R"({"op":"mcm","id":1,"clk":"1234","pt":1583924274639,"mc":[{"rc":[{"atb":[[14,0.23]],"id":18889965},{"atl":[[25,0]],"id":22109331},{"atb":[[3.65,16.38],[3.55,123.25]],"id":13233309}],"img":false,"tv":0,"con":false,"id":"1.170020941"}],"status":0})";
	uint64_t size1 = sizeof(json1) - 1;
	janus::dynamic_buffer dyn_buf(10'000'000);
	janus::betfair::update_state state = {
		.range = &range,
		.filename = "",
		.line = 1,
	};

	uint64_t num_updates =
		janus::betfair::parse_update_stream_json(state, json1, size1, dyn_buf);
	EXPECT_EQ(dyn_buf.size(), sizeof(janus::update) * num_updates);

	// ATL.

	auto pairs =
		find_all_updates_of(janus::update_type::RUNNER_UNMATCHED_ATL, dyn_buf, num_updates);
	EXPECT_EQ(pairs.size(), 1);

	uint64_t price_index;
	double vol;

	EXPECT_EQ(pairs[0].first, 22109331);
	std::tie(price_index, vol) = janus::get_update_runner_unmatched_atl(pairs[0].second);
	EXPECT_EQ(price_index, range.pricex100_to_index(2500));
	EXPECT_DOUBLE_EQ(vol, 0);

	// ATB.

	dyn_buf.reset_read();
	pairs = find_all_updates_of(janus::update_type::RUNNER_UNMATCHED_ATB, dyn_buf, num_updates);
	EXPECT_EQ(pairs.size(), 3);

	EXPECT_EQ(pairs[0].first, 18889965);
	std::tie(price_index, vol) = janus::get_update_runner_unmatched_atb(pairs[0].second);
	EXPECT_EQ(price_index, range.pricex100_to_index(1400));
	EXPECT_DOUBLE_EQ(vol, 0.23);

	EXPECT_EQ(pairs[1].first, 13233309);
	std::tie(price_index, vol) = janus::get_update_runner_unmatched_atb(pairs[1].second);
	EXPECT_EQ(price_index, range.pricex100_to_index(365));
	EXPECT_DOUBLE_EQ(vol, 16.38);

	EXPECT_EQ(pairs[2].first, 13233309);
	std::tie(price_index, vol) = janus::get_update_runner_unmatched_atb(pairs[2].second);
	EXPECT_EQ(price_index, range.pricex100_to_index(355));
	EXPECT_DOUBLE_EQ(vol, 123.25);

	dyn_buf.reset_read();
	pairs = find_all_updates_of(janus::update_type::RUNNER_TRADED_VOL, dyn_buf, num_updates);
	EXPECT_EQ(pairs.size(), 0);

	dyn_buf.reset_read();
	pairs = find_all_updates_of(janus::update_type::RUNNER_LTP, dyn_buf, num_updates);
	EXPECT_EQ(pairs.size(), 0);

	dyn_buf.reset_read();
	pairs = find_all_updates_of(janus::update_type::RUNNER_MATCHED, dyn_buf, num_updates);
	EXPECT_EQ(pairs.size(), 0);

	// LTP, TV, matched volume.

	dyn_buf.reset();
	char json2[] =
		R"({"op":"mcm","id":1,"clk":"1234","pt":1583924323577,"mc":[{"rc":[{"tv":1882.61,"trd":[[8.6,220.32]],"atl":[[8.6,8.18],[9.6,13.12]],"id":17247906},{"tv":874.24,"trd":[[14.5,94.73]],"ltp":14.5,"atl":[[14.5,10.61]],"id":18889965},{"tv":402.81,"trd":[[23,29.64]],"ltp":23,"atb":[[23,0]],"id":22109331}],"img":false,"tv":25130.72,"con":true,"id":"1.170020941"}],"status":0})";
	uint64_t size2 = sizeof(json2) - 1;

	num_updates = janus::betfair::parse_update_stream_json(state, json2, size2, dyn_buf);
	EXPECT_EQ(dyn_buf.size(), sizeof(janus::update) * num_updates);

	// Runner TV.

	pairs = find_all_updates_of(janus::update_type::RUNNER_TRADED_VOL, dyn_buf, num_updates);
	EXPECT_EQ(pairs.size(), 3);

	EXPECT_EQ(pairs[0].first, 17247906);
	EXPECT_DOUBLE_EQ(get_update_runner_traded_vol(pairs[0].second), 1882.61);

	EXPECT_EQ(pairs[1].first, 18889965);
	EXPECT_DOUBLE_EQ(get_update_runner_traded_vol(pairs[1].second), 874.24);

	EXPECT_EQ(pairs[2].first, 22109331);
	EXPECT_DOUBLE_EQ(get_update_runner_traded_vol(pairs[2].second), 402.81);

	// Runner LTP.

	dyn_buf.reset_read();
	pairs = find_all_updates_of(janus::update_type::RUNNER_LTP, dyn_buf, num_updates);
	EXPECT_EQ(pairs.size(), 2);

	EXPECT_EQ(pairs[0].first, 18889965);
	EXPECT_DOUBLE_EQ(get_update_runner_ltp(pairs[0].second), range.pricex100_to_index(1450));

	EXPECT_EQ(pairs[1].first, 22109331);
	EXPECT_DOUBLE_EQ(get_update_runner_ltp(pairs[1].second), range.pricex100_to_index(2300));

	// Matched volume.

	dyn_buf.reset_read();
	pairs = find_all_updates_of(janus::update_type::RUNNER_MATCHED, dyn_buf, num_updates);
	EXPECT_EQ(pairs.size(), 3);

	EXPECT_EQ(pairs[0].first, 17247906);
	std::tie(price_index, vol) = janus::get_update_runner_matched(pairs[0].second);
	EXPECT_EQ(price_index, range.pricex100_to_index(860));
	EXPECT_DOUBLE_EQ(vol, 220.32);

	EXPECT_EQ(pairs[1].first, 18889965);
	std::tie(price_index, vol) = janus::get_update_runner_matched(pairs[1].second);
	EXPECT_EQ(price_index, range.pricex100_to_index(1450));
	EXPECT_DOUBLE_EQ(vol, 94.73);

	EXPECT_EQ(pairs[2].first, 22109331);
	std::tie(price_index, vol) = janus::get_update_runner_matched(pairs[2].second);
	EXPECT_EQ(price_index, range.pricex100_to_index(2300));
	EXPECT_DOUBLE_EQ(vol, 29.64);

	// Matched volume, prices offset from valid prices.

	dyn_buf.reset();
	char json3[] =
		R"({"op":"mcm","id":1,"clk":"1234","pt":1583924323577,"mc":[{"rc":[{"tv":1882.61,"trd":[[8.65,220.32]],"atl":[[8.6,8.18],[9.6,13.12]],"id":17247906},{"tv":874.24,"trd":[[14.539,94.73]],"ltp":14.5,"atl":[[14.5,10.61]],"id":18889965},{"tv":402.81,"trd":[[23.777,29.64]],"ltp":23,"atb":[[23,0]],"id":22109331}],"img":false,"tv":25130.72,"con":true,"id":"1.170020941"}],"status":0})";
	uint64_t size3 = sizeof(json3) - 1;

	num_updates = janus::betfair::parse_update_stream_json(state, json3, size3, dyn_buf);
	EXPECT_EQ(dyn_buf.size(), sizeof(janus::update) * num_updates);

	pairs = find_all_updates_of(janus::update_type::RUNNER_MATCHED, dyn_buf, num_updates);
	EXPECT_EQ(pairs.size(), 3);

	EXPECT_EQ(pairs[0].first, 17247906);
	std::tie(price_index, vol) = janus::get_update_runner_matched(pairs[0].second);
	EXPECT_EQ(price_index, range.pricex100_to_index(860));
	EXPECT_DOUBLE_EQ(vol, 220.32);

	EXPECT_EQ(pairs[1].first, 18889965);
	std::tie(price_index, vol) = janus::get_update_runner_matched(pairs[1].second);
	EXPECT_EQ(price_index, range.pricex100_to_index(1450));
	EXPECT_DOUBLE_EQ(vol, 94.73);

	EXPECT_EQ(pairs[2].first, 22109331);
	std::tie(price_index, vol) = janus::get_update_runner_matched(pairs[2].second);
	EXPECT_EQ(price_index, range.pricex100_to_index(2300));
	EXPECT_DOUBLE_EQ(vol, 29.64);
}
} // namespace
