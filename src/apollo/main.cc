#include "basic1.hh"
#include "janus.hh"

#include <iomanip>
#include <iostream>
#include <string>

auto main(int argc, char** argv) -> int
{
	if (argc < 2) {
		std::cerr << "usage: " << argv[0] << " [strategy name]" << std::endl;
		return 1;
	}

	std::string strat_name = argv[1];
	if (strat_name == "basic1") {
		janus::apollo::basic1 s;
		double pl = s.run();
		std::cout << std::fixed << "P/L = " << pl << std::endl;
	} else {
		std::cerr << "Unknown strategy '" << strat_name << "'??" << std::endl;
		return 1;
	}

	return 0;
}
