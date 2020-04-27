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

	janus::betfair::stream stream(session);
	stream.market_subscribe(config);

	while (true) {
		int size;
		std::cout << stream.read_next_line(size) << std::endl;
	}

	session.logout();

	return 0;
}
