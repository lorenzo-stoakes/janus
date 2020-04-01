#include "janus.hh"

#include <iostream>

// Neptune is a tool for converting existing JSON files to a binary format for
// processing by janus.

// In its initial, skeleton, implementation the tool will process only meta JSON
// files.

auto main(int argc, char** argv) -> int
{
	if (argc < 3) {
		std::cerr << "usage: " << argv[0] << // NOLINT
			" [output file] [meta json file(s)]..." << std::endl;
		return 1;
	}

	return 0;
}
