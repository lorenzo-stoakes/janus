#include "janus.hh"

#include <cstdlib>
#include <iostream>
#include <string>

auto main(int argc, char** argv) -> int
{
	if (argc < 2) {
		std::cerr << "usage: " << argv[0] << " [market ID]" << std::endl;
		return 1;
	}

	uint64_t id = ::atoll(argv[1]);
	try {
		janus::config config = janus::parse_config();
		janus::dynamic_buffer dyn_buf = janus::read_market_updates(config, id);
		uint64_t i = 0;
		while (dyn_buf.read_offset() != dyn_buf.size()) {
			janus::update& u = dyn_buf.read<janus::update>();
			std::cout << i << ": " << janus::betfair::print_update(u) << std::endl;
			i++;
		}

	} catch (std::exception& e) {
		std::cerr << "ERROR: " << e.what() << std::endl;
		return 1;
	}

	return 0;
}
