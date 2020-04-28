#include "janus.hh"

#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>
#include <unordered_map>

//#define META

// A simple metadata -> binary tool for now so we can use this + a script to
// extract legacy metadata to binary format.

static constexpr uint64_t DYN_BUFFER_MAX_SIZE = 1'000'000'000;
static constexpr uint64_t MARKET_DYN_BUFFER_MAX_SIZE = 100'000'000;

static auto file_exists(std::string path) -> bool
{
	std::ifstream f(path);
	return f.good();
}

auto parse_meta(const char* filename, janus::dynamic_buffer& dyn_buf) -> bool
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

auto extract_by_market(janus::dynamic_buffer& dyn_buf, uint64_t num_updates)
	-> std::unordered_map<uint64_t, std::vector<janus::update>>
{
	std::unordered_map<uint64_t, std::vector<janus::update>> map;

	if (num_updates < 2) {
		std::cerr << "Only " << num_updates << " updates, expected minimum 2?" << std::endl;
		throw std::runtime_error("Unexpected failure");
	}

	// Read first update, HAS to be a timestamp.
	janus::update& first = dyn_buf.read<janus::update>();
	if (first.type != janus::update_type::TIMESTAMP) {
		std::cerr << "First update is of type " << janus::update_type_str(first.type)
			  << " not expected timestamp." << std::endl;
		throw std::runtime_error("Unexpected failure");
	}
	uint64_t timestamp = janus::get_update_timestamp(first);

	// Read second update, HAS to be a timestamp.
	janus::update& second = dyn_buf.read<janus::update>();
	if (second.type != janus::update_type::MARKET_ID) {
		std::cerr << "Second update is of type " << janus::update_type_str(second.type)
			  << " not expected market ID." << std::endl;
		throw std::runtime_error("Unexpected failure");
	}
	uint64_t market_id = janus::get_update_market_id(second);

	for (uint64_t i = 2; i < num_updates; i++) {
		janus::update& u = dyn_buf.read<janus::update>();
		if (u.type == janus::update_type::MARKET_ID) {
			market_id = janus::get_update_market_id(u);

			// When switching markets make sure we record the
			// timestamp. This might result in duplicated timestamps
			// but avoids missing ones.
			map[market_id].push_back(janus::make_timestamp_update(timestamp));

			continue;
		}

		if (u.type == janus::update_type::TIMESTAMP)
			timestamp = janus::get_update_timestamp(u);

		map[market_id].push_back(u);
	}

	return map;
}

auto output_stream(janus::dynamic_buffer& dyn_buf, uint64_t num_updates, std::string destdirname)
	-> bool
{
	try {
		auto map = extract_by_market(dyn_buf, num_updates);

		for (auto [market_id, updates] : map) {
			std::string path = destdirname + "/" + std::to_string(market_id) + ".jan";

			if (file_exists(path))
				std::cerr << "Stream output file " << path
					  << " already exists, appending." << std::endl;

			if (auto file = std::ofstream(path, std::ios::binary | std::ios::app)) {
				file.write(reinterpret_cast<char*>(updates.data()),
					   updates.size() * sizeof(janus::update));
			} else {
				std::cerr << "Cannot open " << path << " for writing?" << std::endl;
				return false;
			}
		}

	} catch (std::exception& e) {
		std::cerr << "ERROR: " << e.what() << std::endl;
		return false;
	}

	return true;
}

auto parse_stream(janus::dynamic_buffer& dyn_buf, const char* filename, uint64_t& num_updates)
	-> bool
{
	auto file = std::ifstream(filename);
	if (!file) {
		std::cerr << "Couldn't open " << filename << std::endl;
		return false;
	}

	num_updates = 0;

	janus::betfair::price_range range;
	janus::betfair::update_state state = {
		.range = &range,
		.filename = filename,
		.line = 1,
	};

	std::string line;
	while (std::getline(file, line)) {
		if (line.empty() || line[0] == '\0')
			continue;

		num_updates += janus::betfair::parse_update_stream_json(
			state, reinterpret_cast<char*>(line.data()), line.size(), dyn_buf);
	}

	std::cout << num_updates << std::endl;

	return true;
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

	janus::dynamic_buffer dyn_buf(DYN_BUFFER_MAX_SIZE);

#ifdef META
	if (file_exists(dest)) {
		std::cerr << "Destination file " << dest << " already exists?" << std::endl;
		return 1;
	}

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
#else
	if (!file_exists(dest)) {
		std::cerr << "Destination directory " << dest << " doesn't exist?" << std::endl;
		return 1;
	}

	uint64_t num_updates = 0;

	if (!parse_stream(dyn_buf, source.c_str(), num_updates)) {
		std::cerr << "Couldn't parse stream data from " << source << std::endl;
		return 1;
	}

	// In the case of stream data the destination specifies the directory to
	// write data files into.
	if (!output_stream(dyn_buf, num_updates, dest)) {
		std::cerr << "Couldn't output stream data from " << source << " into directory "
			  << dest << std::endl;
		return 1;
	}
#endif

	return 0;
}
