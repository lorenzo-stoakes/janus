#include "janus.hh"

#include "sajson.hh"
#include <iostream>
#include <string>
#include <vector>

// This is a hacked up skeleton version for testing purposes, initially we are simply logging in to
// betfair and performing simple queries.

// Kinda hacky means of obtaining market IDs for specific filter.
static auto get_market_ids(janus::betfair::session& session) -> std::vector<std::string>
{
	std::string filter_json =
		R"({"eventTypeIds":["7"],"marketTypeCodes":["WIN"],"marketCountries":["US"]})";

	std::string market_cat_json = R"({"filter":)" + filter_json + R"(,"maxResults":1000})";
	std::string response = session.api("listMarketCatalogue", market_cat_json);

	sajson::document doc = janus::internal::parse_json("", const_cast<char*>(response.data()),
							   response.size());
	const sajson::value& root = doc.get_root();

	std::vector<std::string> ret;
	uint64_t count = root.get_length();
	for (uint64_t i = 0; i < count; i++) {
		sajson::value el = root.get_array_element(i);

		ret.push_back(el.get_value_of_key(sajson::literal("marketId")).as_cstring());
	}

	return ret;
}

auto main(int argc, char** argv) -> int
{
	janus::tls::rng rng;
	rng.seed();
	janus::config config = janus::parse_config();

	janus::betfair::session session(rng, config);
	session.load_certs();

	session.login();

	auto market_ids = get_market_ids(session);

	// Now connect to stream API and output connection ID.
	janus::betfair::stream stream(session);

	std::cout << stream.connection_id() << std::endl;

	std::string stream_filter_json = R"({"marketIds":[)";
	for (uint64_t i = 0; i < market_ids.size() - 1; i++) {
		stream_filter_json += "\"";
		stream_filter_json += market_ids[i];
		stream_filter_json += "\",";
	}

	stream_filter_json += "\"" + market_ids[market_ids.size() - 1] + "\"]}";

	std::string data_filter_json =
		R"({"ladderLevels": 10,"fields":["EX_ALL_OFFERS","EX_TRADED","EX_TRADED_VOL","EX_LTP","EX_MARKET_DEF"]})";
	stream.market_subscribe(stream_filter_json, data_filter_json);

	while (true) {
		int size;
		std::cout << stream.read_next_line(size) << std::endl;
	}

	session.logout();

	return 0;
}
