#include "janus.hh"

#include <gtest/gtest.h>
#include <stdexcept>

namespace
{
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
} // namespace
