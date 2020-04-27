#include "janus.hh"

#include "sajson.hh"
#include <iostream>
#include <string>
#include <vector>

// This is a hacked up skeleton version for testing purposes, initially we are simply logging in to
// betfair and performing simple queries.

auto main(int argc, char** argv) -> int
{
	janus::tls::rng rng;
	rng.seed();
	janus::config config = janus::parse_config();

	janus::betfair::session session(rng, config);
	session.load_certs();

	session.login();

	std::string filter_json =
		R"({"eventTypeIds":["7"],"marketTypeCodes":["WIN"],"marketCountries":["US"]})";

	auto market_ids = janus::betfair::get_market_ids(session, filter_json);

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
