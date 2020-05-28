#include "basic1.hh"
#include "catch1.hh"
#include "janus.hh"
#include "question1.hh"

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
		s.run();
	} else if (strat_name == "catch1") {
		janus::apollo::init_configs_catch();
		janus::apollo::catch1 s;
		s.run();
	} else if (strat_name == "question1") {
		janus::apollo::init_configs_question();
		janus::apollo::question1 s;
		s.run();
	} else {
		std::cerr << "Unknown strategy '" << strat_name << "'??" << std::endl;
		return 1;
	}

	return 0;
}
