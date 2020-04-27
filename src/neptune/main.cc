#include "janus.hh"

#include <fstream>
#include <iostream>
#include <string>

// A simple metadata -> binary tool for now so we can use this + a script to
// extract legacy metadata to binary format.

static constexpr uint64_t DYN_BUFFER_MAX_SIZE = 500'000'000;

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

static auto file_exists(std::string path) -> bool
{
	std::ifstream f(path);
	return f.good();
}

auto main(int argc, char** argv) -> int
{
	if (argc != 3) {
		std::cerr << "Usage: " << argv[0] << " [source file] [destination file]"
			  << std::endl;
		return 1;
	}

	std::string source = argv[1];
	std::string dest = argv[2];

	if (file_exists(dest)) {
		std::cerr << "Destination file " << dest << " already exists?" << std::endl;
		return 1;
	}

	janus::dynamic_buffer dyn_buf(DYN_BUFFER_MAX_SIZE);

	if (!parse_meta(source.c_str(), dyn_buf)) {
		std::cerr << "Couldn't parse metadata from " << source << std::endl;
		return 1;
	}

	if (auto file = std::ofstream(dest, std::ios::binary)) {
		file.write(reinterpret_cast<char*>(dyn_buf.data()), dyn_buf.size());
	} else {
		std::cerr << "Couldn't open " + dest + " for writing" << std::endl;
		return 1;
	}

	return 0;
}
