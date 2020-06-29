#include "janus.hh"

#include <cstdint>
#include <iostream>
#include <string>

void place_order(uint64_t market_id, uint64_t runner_id, double price, double quantity,
		 bool is_back)
{
	janus::tls::rng rng;
	rng.seed();
	janus::config config = janus::parse_config();
	janus::betfair::session conn(rng, config);
	conn.load_certs();
	conn.login();

	janus::bet bet(runner_id, price, quantity, is_back);
	auto res = janus::betfair::place_order(conn, market_id, bet);
	if (res.result != janus::betfair::result_type::SUCCESS) {
		std::cerr << "ERROR: Status: " << res.status << " error codes: " << res.error_code
			  << ", " << res.report.error_code << std::endl;
		return;
	}

	auto& report = res.report;

	std::cout << "Success! Bet ID: " << report.bet_id
		  << " order status: " << report.order_status;
	if (report.average_price_matched > 0)
		std::cout << " ~px: " << report.average_price_matched
			  << " matched: " << report.size_matched;

	std::cout << std::endl;
}

auto main(int argc, char** argv) -> int
{
	// As an initial skeleton exploration we are making mars simply execute
	// orders from the command line.

	if (argc < 5) {
		std::cerr << "usage: " << argv[0]
			  << " [market id] [runner id] [price] [quantity] [back/lay]" << std::endl;
		return 1;
	}

	// If these fail a std::invalid_argument exception is thrown, since this is
	uint64_t market_id = std::stoll(argv[1]);
	uint64_t runner_id = std::stoll(argv[2]);
	double price = std::stod(argv[3]);
	double quantity = std::stod(argv[4]);
	std::string side = argv[5];
	bool is_back;
	if (side == "BACK") {
		is_back = true;
	} else if (side == "LAY") {
		is_back = false;
	} else {
		std::cerr << "Unrecognised side " << side << std::endl;
		return 1;
	}

	place_order(market_id, runner_id, price, quantity, is_back);

	return 0;
}
