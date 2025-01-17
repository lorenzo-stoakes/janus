#include "janus.hh"

#include "unistd.h"
#include <cstdint>
#include <cstring>
#include <fstream>
#include <iostream>
#include <string>

#include <snappy.h>

// Set to print each update parsed from files.
//#define PRINT_UPDATES
// Set to skip saving altogether.
#define SKIP_SAVE
// Continue running when an error occurs in parsing updates.
//#define CONTINUE_ON_ERROR

// Show progress as updating by updating the same output line.
#define SHOW_PROGRESS
// Show number of updates on exit.
#define SHOW_NUM_UPDATES

constexpr uint64_t UNIVERSE_SIZE = 30000;

// Checker is a tool for converting existing JSON files to a binary format for
// processing by janus and parsing it (if update stream data) in a universe for
// the purposes of checking that the logic is functioning correctly.

static constexpr uint64_t DYN_BUFFER_MAX_SIZE = 500'000'000;

#ifdef SHOW_PROGRESS
static void clear_line()
{
	std::cerr << "\r";
	for (int i = 0; i < 100; i++) // NOLINT: Not magical.
		std::cerr << " ";
	std::cerr << "\r" << std::flush;
}
#endif

static auto parse_meta(const char* filename, janus::dynamic_buffer& dyn_buf) -> bool
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

static auto parse_update_stream(const janus::betfair::price_range& range, const char* filename,
				janus::dynamic_buffer& dyn_buf, uint64_t& num_updates,
				janus::betfair::universe<UNIVERSE_SIZE>& universe) -> bool
{
	janus::betfair::update_state state = {
		.range = &range,
		.filename = filename,
		.line = 1,
	};

	if (auto file = std::ifstream(filename)) {
		universe.set_last_timestamp(0);

		std::string line;
		while (std::getline(file, line)) {
			// Some lines in 'all' folder contain corrupted data :'( skip these.
			if (line.empty() || line[0] == '\0')
				continue;

			num_updates += janus::betfair::parse_update_stream_json(
				state, reinterpret_cast<char*>(line.data()), line.size(), dyn_buf);

			try {
				while (dyn_buf.read_offset() != dyn_buf.size()) {
					const auto update = dyn_buf.read<janus::update>();

#ifdef PRINT_UPDATES
					std::cout << state.line - 1 << ": "
						  << update_type_str(update.type);
					if (update.type ==
						    janus::update_type::RUNNER_UNMATCHED_ATL ||
					    update.type ==
						    janus::update_type::RUNNER_UNMATCHED_ATB) {
						std::cout << ": key as price = "
							  << janus::betfair::price_range::
								     index_to_price(update.key);
						std::cout << " value as double = "
							  << update.value.d;
					} else if (update.type == janus::update_type::RUNNER_ID) {
						std::cout << ": " << update.value.u;
					}
					std::cout << std::endl;
#endif
					universe.apply_update(update);
				}
			} catch (std::exception& e) {
				// We would have already updated the line number.
				uint64_t line_num = state.line - 1;

				std::cout << "ERROR: " << filename << ":" << line_num << ": "
					  << e.what() << std::endl;
#ifndef CONTINUE_ON_ERROR
				return false;
#endif
			}
		}
	} else {
		std::cerr << "Couldn't open " << filename << std::endl;
		return false;
	}

	return true;
}

static auto save_data(const std::string& output_filename, const janus::dynamic_buffer& dyn_buf)
	-> bool
{
#ifdef SKIP_SAVE
	return true;
#endif

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

	auto ptr = std::make_unique<janus::betfair::universe<UNIVERSE_SIZE>>();
	auto& universe = *ptr;

	for (int i = 1 + arg_offset; i < argc; i++) {
		const char* filename = argv[i];

#ifdef SHOW_PROGRESS
		clear_line();
		std::cerr << std::to_string(i) << "/" << std::to_string(argc - 1) << ": "
			  << filename << std::flush;
#endif

		if ((meta && !parse_meta(filename, dyn_buf)) ||
		    (!meta &&
		     !parse_update_stream(range, filename, dyn_buf, num_updates, universe)))
			return 1;

		// If we've used more than 90% of available buffer space, save it.
		if (dyn_buf.size() > 0.9 * DYN_BUFFER_MAX_SIZE) { // NOLINT: Not magical.
			if (!save_data(output_filename, dyn_buf))
				return 1;
			dyn_buf.reset();
		}
	}

	if (!save_data(output_filename, dyn_buf))
		return 1;

#ifdef SHOW_PROGRESS
	std::cerr << std::endl;
#endif

#ifdef SHOW_NUM_UPDATES
	if (!meta)
		std::cout << num_updates << " updates" << std::endl;
#endif

	return 0;
}
