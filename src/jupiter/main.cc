#include "janus.hh"

#include <iostream>

// This is a hacked up skeleton version for testing purposes, initially we are simply logging in to
// betfair

auto main(int argc, char** argv) -> int
{
	janus::tls::rng rng;
	rng.seed();
	janus::config config = janus::parse_config();

	janus::betfair::session session(rng, config);
	session.load_certs();

	session.login();

	std::string filter_json =
		R"({"filter":{"eventTypeIds":["7"],"marketTypeCodes":["WIN"]},"maxResults":1000})";
	std::string response = session.api("listMarketCatalogue", filter_json);
	std::cout << response << std::endl;

	// Now connect to stream API and output connection ID.
	auto [conn_id, conn] = session.make_stream_connection();
	std::cout << conn_id << std::endl;

	session.logout();

	return 0;
}
