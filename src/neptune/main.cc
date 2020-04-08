#include "janus.hh"

#include <cstdint>
#include <fstream>
#include <iostream>
#include <string>

// Neptune is a tool for converting existing JSON files to a binary format for
// processing by janus.

// In its initial, skeleton, implementation the tool will process only meta JSON
// files.

static constexpr uint64_t DYN_BUFFER_MAX_SIZE = 500'000'000;

auto main(int argc, char** argv) -> int
{
	if (argc < 3) {
		std::cerr << "usage: " << argv[0] << " [output file] [meta json file(s)]..."
			  << std::endl;
		return 1;
	}

	janus::dynamic_buffer dyn_buf(DYN_BUFFER_MAX_SIZE);

	for (int i = 2; i < argc; i++) {
		const char* filename = argv[i];
		std::string json;
		uint64_t size;

		if (auto file = std::ifstream(filename, std::ios::ate)) {
			size = file.tellg();

			json = std::string(size, '\0');
			file.seekg(0);

			if (!file.read(&json[0], size)) {
				std::cerr << "Couldn't read " << filename << std::endl;
				return 1;
			}
		} else {
			std::cerr << "Couldn't open " << filename << std::endl;
			return 1;
		}

		janus::betfair::parse_meta_json(filename, reinterpret_cast<char*>(json.data()),
						size, dyn_buf);
	}

	std::string output_filename = argv[1];
	if (auto file = std::fstream(output_filename, std::ios::out | std::ios::binary)) {
		file.write(reinterpret_cast<const char*>(dyn_buf.data()), dyn_buf.size());
	} else {
		std::cerr << "Couldn't open " << output_filename << " for reading" << std::endl;
		return 1;
	}

	return 0;
}
