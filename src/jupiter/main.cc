#include "janus.hh"

#include "sajson.hh"
#include <atomic>
#include <iostream>
#include <signal.h>
#include <string>
#include <vector>

// This is a hacked up skeleton version for testing purposes, initially we are simply logging in to
// betfair and performing simple queries.

std::atomic<bool> interrupted{false};

static void handle_signal(int)
{
	interrupted.store(true);
}

// Courtesy of https://stackoverflow.com/a/4250601
static void add_signal_handler()
{
	struct sigaction sa;
	memset(&sa, 0, sizeof(sa));
	sa.sa_handler = handle_signal;
	::sigfillset(&sa.sa_mask);
	::sigaction(SIGINT, &sa, nullptr);
}

auto main(int argc, char** argv) -> int
{
	add_signal_handler();

	janus::tls::rng rng;
	rng.seed();

	janus::config config = janus::parse_config();

	janus::betfair::session session(rng, config);
	session.load_certs();
	session.login();

	janus::betfair::stream stream(session);

	std::cout << "Getting metadata..." << std::endl;
	std::string meta = stream.market_subscribe(config);
	// Not doing anything with it yet.
	(void)meta;

	std::cout << "Done. Starting stream..." << std::endl;
	while (true) {
		if (interrupted.load())
			break;

		int size;
		std::cout << stream.read_next_line(size) << std::endl;
	}

	std::cout << "Logging out..." << std::endl;
	session.logout();

	return 0;
}
