#include "janus.hh"

#include "unistd.h"
#include <cstdint>
#include <cstring>
#include <fstream>
#include <iostream>
#include <string>

#include <snappy.h>

// Neptune is a tool for converting existing JSON files to a binary format for
// processing by janus.

// This version is a hacky, terrible and over-allocating jumble designed for
// testing out the parsing logic of janus and getting an idea of performance,
// compression ratio and correctness when parsing real data.

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

static bool parse_update_stream(const janus::betfair::price_range& range, const char* filename,
				janus::dynamic_buffer& dyn_buf, uint64_t& num_updates)
{
	janus::betfair::update_state state = {
		.range = &range,
		.filename = filename,
		.line = 1,
	};

	if (auto file = std::ifstream(filename)) {
		std::string line;
		while (std::getline(file, line)) {
			num_updates += janus::betfair::parse_update_stream_json(
				state, reinterpret_cast<char*>(line.data()), line.size(), dyn_buf);
		}
	} else {
		std::cerr << "Couldn't open " << filename << std::endl;
		return false;
	}

	return true;
}

static bool save_data(const std::string& output_filename, const janus::dynamic_buffer& dyn_buf)
{
	// Note that we are saving and appending at arbitrary points so this
	// compression is useless, however since we are implementing this for
	// benchmark and test purposes at this stage it's fine.

	std::string compressed;
	snappy::Compress(reinterpret_cast<char*>(dyn_buf.data()), dyn_buf.size(), &compressed);

	if (auto file = std::ofstream(output_filename, std::ios::binary | std::ios::app)) {
		file << compressed;
	} else {
		std::cerr << "Couldn't open " << output_filename << " for reading" << std::endl;
		return false;
	}

	return true;
}

auto main(int argc, char** argv) -> int
{
	if (argc < 2) {
		std::cerr << "usage: " << argv[0] << " [--meta] [meta json file(s)]..."
			  << std::endl;
		return 1;
	}

	janus::dynamic_buffer dyn_buf(DYN_BUFFER_MAX_SIZE);
	janus::betfair::price_range range;

	int arg_offset = 0;
	bool meta = false;
	if (::strncmp(argv[1], "--meta", sizeof("--meta") - 1) == 0) {
		meta = true;
		arg_offset = 1;
	}

	// Default to out.jan to avoid fat fingers causing JSON files to get corrupted...
	std::string output_filename = "out.jan";

	// Delete output file if exists so we can append to it.
	::unlink(output_filename.c_str());

	uint64_t num_updates = 0;

	for (int i = 1 + arg_offset; i < argc; i++) {
		const char* filename = argv[i];

		clear_line();
		std::cout << std::to_string(i) << "/" << std::to_string(argc - 1) << ": "
			  << filename << std::flush;

		if ((meta && !parse_meta(filename, dyn_buf)) ||
		    (!meta && !parse_update_stream(range, filename, dyn_buf, num_updates)))
			return 1;

		// If we've used more than 90% of available buffer space, save it.
		if (dyn_buf.size() > 0.9 * DYN_BUFFER_MAX_SIZE) {
			save_data(output_filename, dyn_buf);
			dyn_buf.reset();
		}
	}

	save_data(output_filename, dyn_buf);

	std::cout << std::endl;

	if (!meta)
		std::cout << num_updates << " updates" << std::endl;

	return 0;
}
