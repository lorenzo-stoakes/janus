#include "janus.hh"

#include <cstdint>
#include <cstring>
#include <fstream>
#include <iostream>
#include <string>

// Neptune is a tool for converting existing JSON files to a binary format for
// processing by janus.

// In its initial, skeleton, implementation the tool will process only meta JSON
// files.

static constexpr uint64_t DYN_BUFFER_MAX_SIZE = 500'000'000;

static void clear_line()
{
	std::cout << "\r";
	for (int i = 0; i < 100; i++) // NOLINT: Not magical.
		std::cout << " ";
	std::cout << "\r" << std::flush;
}

static bool parse_meta(const char* filename, janus::dynamic_buffer& dyn_buf)
{
	std::string json;
	uint64_t size;

	if (auto file = std::ifstream(filename, std::ios::ate)) {
		size = file.tellg();

		json = std::string(size, '\0');
		file.seekg(0);

		if (!file.read(&json[0], size)) {
			std::cerr << "Couldn't read " << filename << std::endl;
			return false;
		}
	} else {
		std::cerr << "Couldn't open " << filename << std::endl;
		return false;
	}

	janus::betfair::parse_meta_json(filename, reinterpret_cast<char*>(json.data()), size,
					dyn_buf);

	return true;
}

auto main(int argc, char** argv) -> int
{
	if (argc < 3) {
		std::cerr << "usage: " << argv[0]
			  << " [--meta] [output file] [meta json file(s)]..." << std::endl;
		return 1;
	}

	int arg_offset = 0;
	bool meta = false;
	if (::strncmp(argv[1], "--meta", sizeof("--meta") - 1) == 0) {
		meta = true;
		arg_offset = 1;
	}

	janus::dynamic_buffer dyn_buf(DYN_BUFFER_MAX_SIZE);

	for (int i = 2 + arg_offset; i < argc; i++) {
		const char* filename = argv[i];

		clear_line();
		std::cout << std::to_string(i - 1) << "/" << std::to_string(argc - 2) << ": "
			  << filename << std::flush;

		if (meta && !parse_meta(filename, dyn_buf))
			return 1;
	}

	std::string output_filename = argv[1 + arg_offset];
	if (auto file = std::fstream(output_filename, std::ios::out | std::ios::binary)) {
		file.write(reinterpret_cast<const char*>(dyn_buf.data()), dyn_buf.size());
	} else {
		std::cerr << "Couldn't open " << output_filename << " for reading" << std::endl;
		return 1;
	}

	std::cout << std::endl;
	return 0;
}
