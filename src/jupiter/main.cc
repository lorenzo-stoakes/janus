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
	std::cout << session.session_token() << std::endl;
	session.logout();

	return 0;
}
