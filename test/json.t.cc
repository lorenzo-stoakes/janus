#include "dynamic_buffer.hh"
#include "json.hh"
#include "meta.hh"
#include "parse.hh"

#include <cstdint>
#include <cstring>
#include <gtest/gtest.h>
#include <string>

#include "sample_data.hh"

namespace
{
// Test that the internal function parse_json() correctly returns an
// sajson::document or throws an exception on invalid input.
TEST(json_test, parse_json)
{
	// An empty input should throw.
	char buf1[] = "";
	EXPECT_THROW(janus::internal::parse_json("", buf1, sizeof(buf1) - 1),
		     janus::json_parse_error);

	// Unclosed brackets should too.
	char buf2[] = "[[({";
	EXPECT_THROW(janus::internal::parse_json("", buf2, sizeof(buf2) - 1),
		     janus::json_parse_error);

	// We'll leave further invalid input checks to the sajson-specific tests.

	// Check a simple input.
	char buf3[] = "[{\"x\":3}]";
	sajson::document doc = janus::internal::parse_json("", buf3, sizeof(buf3) - 1);
	const sajson::value& root = doc.get_root();

	EXPECT_EQ(root.get_type(), sajson::TYPE_ARRAY);

	EXPECT_EQ(root.get_array_element(0)
			  .get_value_of_key(sajson::literal("x"))
			  .get_integer_value(),
		  3);
}

// Test that the remove_outer_array() internal function correctly removes outer
// [] from JSON or if not present leaves the JSON as-is.
TEST(json_test, remove_outer_array)
{
	char invalid_buf[] = "123";

	uint64_t size = 0;
	// Anything less than size 3 should fail.
	EXPECT_THROW(janus::internal::remove_outer_array(invalid_buf, size), std::runtime_error);
	size = 1;
	EXPECT_THROW(janus::internal::remove_outer_array(invalid_buf, size), std::runtime_error);
	size = 2;
	EXPECT_THROW(janus::internal::remove_outer_array(invalid_buf, size), std::runtime_error);

	// Now check a normal case.
	char buf1[] = R"([{"x":3}])";
	uint64_t buf1_orig_size = sizeof(buf1) - 1;
	size = buf1_orig_size;
	char* ret1 = janus::internal::remove_outer_array(buf1, size);
	EXPECT_EQ(size, buf1_orig_size - 2);
	for (uint64_t i = 1; i < buf1_orig_size - 1; i++) {
		EXPECT_EQ(buf1[i], ret1[i - 1]);
	}

	// Now check a normal case with trailing whitespace.
	char buf2[] = R"([{"x":3}]      )";
	uint64_t buf2_orig_size = sizeof(buf2) - 1;
	size = buf2_orig_size;
	char* ret2 = janus::internal::remove_outer_array(buf2, size);
	// Expect [] and trailing whitespace to be removed.
	EXPECT_EQ(size, buf2_orig_size - 2 - 6);
	for (uint64_t i = 1; i < 8; i++) {
		EXPECT_EQ(buf2[i], ret2[i - 1]);
	}

	// Input JSON without an outer array should remain the same.
	char buf3[] = R"({"x":3, "y": 1.23456, "z": ["ohello!"])";
	uint64_t buf3_orig_size = sizeof(buf3) - 1;
	size = buf3_orig_size;
	char* ret3 = janus::internal::remove_outer_array(buf3, size);
	EXPECT_EQ(size, buf3_orig_size);
	EXPECT_EQ(std::strcmp(buf3, ret3), 0);

	// Invalid JSON without closing brace should raise an error here.
	char buf4[] = R"([{"x":3})";
	size = sizeof(buf4) - 1;
	EXPECT_THROW(janus::internal::remove_outer_array(buf4, size), std::runtime_error);
}

// Test that we can parse a betfair metadata header correctly.
TEST(json_test, betfair_extract_meta_header)
{
	// Most of the functionality underlying all of this is implemented and
	// tested elsewhere so there is no need to go too deep - we know we can
	// parse JSON, we know we can parse the market ID and start time, we
	// know the dynamic buffer works, so what matters now is to ensure we
	// parse the right fields and put them in the correct place.

	char json[] =
		R"({"marketId":"1.170020941","marketStartTime":"2020-03-11T13:20:00Z","eventType":{"id":"7","name":"Horse Racing"},"competition":{"id":"","name":""},"event":{"id":"29746086","name":"Ling  11th Mar","countryCode":"GB","timezone":"Europe/London","venue":"Lingfield","openDate":"2020-03-11T13:20:00Z"},"runners":[{"selectionId":13233309,"runnerName":"Zayriyan","sortPriority":1},{"selectionId":11622845,"runnerName":"Abel Tasman","sortPriority":2},{"selectionId":17247906,"runnerName":"Huddle","sortPriority":3},{"selectionId":12635885,"runnerName":"Muraaqeb","sortPriority":4}]})";
	uint64_t size = sizeof(json) - 1;

	sajson::document doc = janus::internal::parse_json("", json, size);
	const sajson::value& root = doc.get_root();

	// Won't be larger than the JSON buffer.
	auto dyn_buf = janus::dynamic_buffer(size);

	uint64_t count = janus::internal::betfair_extract_meta_header(root, dyn_buf);
	// Already a multiple of 8.
	EXPECT_EQ(count, sizeof(janus::meta_header));

	auto& header = dyn_buf.read<janus::meta_header>();

	EXPECT_EQ(header.market_id, 170020941);
	EXPECT_EQ(header.event_type_id, 7);
	EXPECT_EQ(header.event_id, 29746086);
	EXPECT_EQ(header.competition_id, 0);
	char timestamp[] = "2020-03-11T13:20:00Z";
	EXPECT_EQ(header.market_start_timestamp,
		  janus::parse_iso8601(timestamp, sizeof(timestamp) - 1));
	EXPECT_EQ(header.num_runners, 4);

	// Try again and make sure count looks correct, add a competition ID.
	char json2[] =
		R"({"marketId":"1.170020941","marketStartTime":"2020-03-11T13:20:00Z","eventType":{"id":"7","name":"Horse Racing"},"competition":{"id":"123456","name":"Foobar"},"event":{"id":"29746086","name":"Ling  11th Mar","countryCode":"GB","timezone":"Europe/London","venue":"Lingfield","openDate":"2020-03-11T13:20:00Z"},"runners":[{"selectionId":13233309,"runnerName":"Zayriyan","sortPriority":1},{"selectionId":11622845,"runnerName":"Abel Tasman","sortPriority":2},{"selectionId":17247906,"runnerName":"Huddle","sortPriority":3},{"selectionId":12635885,"runnerName":"Muraaqeb","sortPriority":4}]})";
	uint64_t size2 = sizeof(json2) - 1;

	sajson::document doc2 = janus::internal::parse_json("", json2, size2);
	const sajson::value& root2 = doc2.get_root();

	uint64_t count2 = janus::internal::betfair_extract_meta_header(root2, dyn_buf);
	EXPECT_EQ(count2, sizeof(janus::meta_header));
	EXPECT_EQ(dyn_buf.size(), 2 * sizeof(janus::meta_header));

	auto& header2 = dyn_buf.read<janus::meta_header>();
	EXPECT_EQ(header2.market_id, 170020941);
	EXPECT_EQ(header2.event_type_id, 7);
	EXPECT_EQ(header2.event_id, 29746086);
	EXPECT_EQ(header2.competition_id, 123456);
	EXPECT_EQ(header2.market_start_timestamp,
		  janus::parse_iso8601(timestamp, sizeof(timestamp) - 1));
	EXPECT_EQ(header2.num_runners, 4);

	// Now eliminate the competition key altogether.
	char json3[] =
		R"({"marketId":"1.170020941","marketStartTime":"2020-03-11T13:20:00Z","eventType":{"id":"7","name":"Horse Racing"},"event":{"id":"29746086","name":"Ling  11th Mar","countryCode":"GB","timezone":"Europe/London","venue":"Lingfield","openDate":"2020-03-11T13:20:00Z"},"runners":[{"selectionId":13233309,"runnerName":"Zayriyan","sortPriority":1},{"selectionId":11622845,"runnerName":"Abel Tasman","sortPriority":2},{"selectionId":17247906,"runnerName":"Huddle","sortPriority":3},{"selectionId":12635885,"runnerName":"Muraaqeb","sortPriority":4}]})";
	uint64_t size3 = sizeof(json3) - 1;

	sajson::document doc3 = janus::internal::parse_json("", json3, size3);
	const sajson::value& root3 = doc3.get_root();

	uint64_t count3 = janus::internal::betfair_extract_meta_header(root3, dyn_buf);
	EXPECT_EQ(count3, sizeof(janus::meta_header));
	EXPECT_EQ(dyn_buf.size(), 3 * sizeof(janus::meta_header));

	auto& header3 = dyn_buf.read<janus::meta_header>();
	EXPECT_EQ(header3.market_id, 170020941);
	EXPECT_EQ(header3.event_type_id, 7);
	EXPECT_EQ(header3.event_id, 29746086);
	EXPECT_EQ(header3.competition_id, 0);
	EXPECT_EQ(header3.market_start_timestamp,
		  janus::parse_iso8601(timestamp, sizeof(timestamp) - 1));
	EXPECT_EQ(header3.num_runners, 4);
}

// Test that we can parse betfair metadata static strings correctly.
TEST(json_test, betfair_extract_meta_static_strings)
{
	char json[] =
		R"({"marketId":"1.170020941","marketName":"1m2f Hcap","marketStartTime":"2020-03-11T13:20:00Z","eventType":{"id":"7","name":"Horse Racing"},"competition":{"id":"123456","name":"Foobar"},"event":{"id":"29746086","name":"Ling  11th Mar","countryCode":"GB","timezone":"Europe/London","venue":"Lingfield","openDate":"2020-03-11T13:20:00Z"},"runners":[{"selectionId":13233309,"runnerName":"Zayriyan","sortPriority":1},{"selectionId":11622845,"runnerName":"Abel Tasman","sortPriority":2},{"selectionId":17247906,"runnerName":"Huddle","sortPriority":3},{"selectionId":12635885,"runnerName":"Muraaqeb","sortPriority":4}],"description":{"marketType":"WIN"}})";
	uint64_t size = sizeof(json) - 1;

	sajson::document doc = janus::internal::parse_json("", json, size);
	const sajson::value& root = doc.get_root();

	// Won't be larger than the JSON buffer.
	auto dyn_buf = janus::dynamic_buffer(size);

	uint64_t count = janus::internal::betfair_extract_meta_static_strings(root, dyn_buf);
	EXPECT_EQ(count, dyn_buf.size());

	std::string_view market_name = dyn_buf.read_string();
	EXPECT_EQ(market_name, "1m2f Hcap");

	std::string_view event_type_name = dyn_buf.read_string();
	EXPECT_EQ(event_type_name, "Horse Racing");

	std::string_view event_name = dyn_buf.read_string();
	EXPECT_EQ(event_name, "Ling  11th Mar");

	std::string_view event_country_code = dyn_buf.read_string();
	EXPECT_EQ(event_country_code, "GB");

	std::string_view event_timezone = dyn_buf.read_string();
	EXPECT_EQ(event_timezone, "Europe/London");

	std::string_view market_type_name = dyn_buf.read_string();
	EXPECT_EQ(market_type_name, "WIN");

	std::string_view venue_name = dyn_buf.read_string();
	EXPECT_EQ(venue_name, "Lingfield");

	std::string_view competition_name = dyn_buf.read_string();
	EXPECT_EQ(competition_name, "Foobar");

	// Missing venue and competition.
	char json2[] =
		R"({"marketId":"1.170020941","marketName":"1m2f Hcap","marketStartTime":"2020-03-11T13:20:00Z","eventType":{"id":"7","name":"Horse Racing"},"competition":null,"event":{"id":"29746086","name":"Ling  11th Mar","countryCode":"GB","timezone":"Europe/London","openDate":"2020-03-11T13:20:00Z"},"runners":[{"selectionId":13233309,"runnerName":"Zayriyan","sortPriority":1},{"selectionId":11622845,"runnerName":"Abel Tasman","sortPriority":2},{"selectionId":17247906,"runnerName":"Huddle","sortPriority":3},{"selectionId":12635885,"runnerName":"Muraaqeb","sortPriority":4}],"description":{"marketType":"WIN"}})";
	uint64_t size2 = sizeof(json2) - 1;

	sajson::document doc2 = janus::internal::parse_json("", json2, size2);
	const sajson::value& root2 = doc2.get_root();

	// Won't be larger than the JSON buffer.
	auto dyn_buf2 = janus::dynamic_buffer(size);

	uint64_t count2 = janus::internal::betfair_extract_meta_static_strings(root2, dyn_buf2);
	EXPECT_EQ(count2, dyn_buf2.size());

	std::string_view market_name2 = dyn_buf2.read_string();
	EXPECT_EQ(market_name2, "1m2f Hcap");

	std::string_view event_type_name2 = dyn_buf2.read_string();
	EXPECT_EQ(event_type_name2, "Horse Racing");

	std::string_view event_name2 = dyn_buf2.read_string();
	EXPECT_EQ(event_name2, "Ling  11th Mar");

	std::string_view event_country_code2 = dyn_buf2.read_string();
	EXPECT_EQ(event_country_code2, "GB");

	std::string_view event_timezone2 = dyn_buf2.read_string();
	EXPECT_EQ(event_timezone2, "Europe/London");

	std::string_view market_type_name2 = dyn_buf2.read_string();
	EXPECT_EQ(market_type_name2, "WIN");

	std::string_view venue_name2 = dyn_buf2.read_string();
	EXPECT_EQ(venue_name2, "");

	std::string_view competition_name2 = dyn_buf2.read_string();
	EXPECT_EQ(competition_name2, "");
}

// Test that the internal betfair_extract_meta_runners() function correctly extracts runner
// metadata.
TEST(json_test, betfair_extract_meta_runners)
{
	std::string json = janus_test::sample_meta_json;

	sajson::document doc =
		janus::internal::parse_json("", reinterpret_cast<char*>(json.data()), json.size());
	const sajson::value& root = doc.get_root();

	// Won't be larger than the JSON buffer.
	auto dyn_buf = janus::dynamic_buffer(json.size());

	// Extract header so we can get a runner count.
	janus::internal::betfair_extract_meta_header(root, dyn_buf);
	auto& header = dyn_buf.read<janus::meta_header>();
	uint64_t num_runners = header.num_runners;
	EXPECT_EQ(num_runners, 12);

	// Now we've extracted number of runners, reset the dynamic buffer for
	// reading runner data.
	dyn_buf.reset();

	sajson::value runners_node = root.get_value_of_key(sajson::literal("runners"));
	uint64_t count = janus::internal::betfair_extract_meta_runners(runners_node, dyn_buf);
	ASSERT_EQ(count, dyn_buf.size());

	for (uint64_t i = 0; i < num_runners; i++) {
		uint64_t runner_id = dyn_buf.read_uint64();
		uint64_t sort_priority = dyn_buf.read_uint64();
		std::string_view runner_name = dyn_buf.read_string();
		std::string_view jockey_name = dyn_buf.read_string();
		std::string_view trainer_name = dyn_buf.read_string();

		EXPECT_EQ(runner_id, janus_test::sample_runner_ids[i]);
		EXPECT_EQ(sort_priority, i + 1);
		EXPECT_EQ(runner_name, janus_test::sample_runner_names[i]);
		EXPECT_EQ(jockey_name, janus_test::sample_jockey_names[i]);
		EXPECT_EQ(trainer_name, janus_test::sample_trainer_names[i]);
	}

	// Now read data without any metadata attached (e.g. jockey, trainer name) and make sure
	// this works correctly.

	char json2[] =
		R"({"marketId":"1.170020941","marketName":"1m2f Hcap","marketStartTime":"2020-03-11T13:20:00Z","eventType":{"id":"7","name":"Horse Racing"},"competition":null,"event":{"id":"29746086","name":"Ling  11th Mar","countryCode":"GB","timezone":"Europe/London","openDate":"2020-03-11T13:20:00Z"},"runners":[{"selectionId":13233309,"runnerName":"Zayriyan","sortPriority":1},{"selectionId":11622845,"runnerName":"Abel Tasman","sortPriority":2,"metadata":{}},{"selectionId":17247906,"runnerName":"Huddle","sortPriority":3,"metadata":null},{"selectionId":12635885,"runnerName":"Muraaqeb","sortPriority":4}],"description":{"marketType":"WIN"}})";
	uint64_t size2 = sizeof(json2) - 1;

	sajson::document doc2 = janus::internal::parse_json("", json2, size2);
	const sajson::value& root2 = doc2.get_root();
	sajson::value runners_node2 = root2.get_value_of_key(sajson::literal("runners"));

	janus::internal::betfair_extract_meta_runners(runners_node2, dyn_buf);

	for (uint64_t i = 0; i < 4; i++) {
		uint64_t runner_id = dyn_buf.read_uint64();
		uint64_t sort_priority = dyn_buf.read_uint64();
		std::string_view runner_name = dyn_buf.read_string();
		std::string_view jockey_name = dyn_buf.read_string();
		std::string_view trainer_name = dyn_buf.read_string();

		EXPECT_EQ(runner_id, janus_test::sample_runner_ids[i]);
		EXPECT_EQ(sort_priority, i + 1);
		EXPECT_EQ(runner_name, janus_test::sample_runner_names[i]);
		EXPECT_EQ(jockey_name.size(), 0);
		EXPECT_EQ(trainer_name.size(), 0);
	}
}
} // namespace
