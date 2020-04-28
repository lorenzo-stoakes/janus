#include "janus.hh"

#include "sajson.hh"
#include "spdlog/spdlog.h"
#include <algorithm>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>
namespace fs = std::filesystem;

static constexpr uint64_t MAX_METADATA_BYTES = 100'000;

// Represents entry in neptune.db database file.
struct db_entry
{
	db_entry() : file_id{0}, last_updated{0}, last_offset{0} {}
	explicit db_entry(uint64_t id) : file_id{id}, last_updated{0}, last_offset{0} {}

	uint64_t file_id;
	uint64_t last_updated;
	uint64_t last_offset;
};

// Does the file at the specified path exist?
static auto file_exists(std::string path) -> bool
{
	std::ifstream f(path);
	return f.good();
}

// Retrieve list of JSON files. By convention the filename is [ms since
// epoch].js both for metadata and stream data.
auto get_file_id_list(const janus::config& config) -> std::vector<uint64_t>
{
	std::vector<uint64_t> ret;

	std::string meta_dir_path = config.json_data_root + "/meta";
	for (const auto& entry : fs::directory_iterator(meta_dir_path)) {
		std::string num_str = entry.path().stem().string();
		ret.push_back(std::stoll(num_str));
	}

	// Since these are formated as ms since epoch, sorting will give us the
	// files in correct chronological order.
	std::sort(ret.begin(), ret.end());
	return ret;
}

// Read database file containing information about imported data.
auto read_db(const janus::config& config) -> std::unordered_map<uint64_t, db_entry>
{
	std::string path = config.binary_data_root + "/neptune.db";

	std::vector<db_entry> entries;
	if (file_exists(path)) {
		auto file = std::ifstream(path, std::ios::binary | std::ios::ate);
		if (!file)
			throw std::runtime_error(std::string("Cannot open ") + path);

		uint64_t size = file.tellg();
		entries = std::vector<db_entry>(size / sizeof(db_entry));

		file.seekg(0);
		file.read(reinterpret_cast<char*>(entries.data()), size);
	}

	// TODO(lorenzo): Probably not the most efficient method...
	std::unordered_map<uint64_t, db_entry> map;
	for (auto e : entries) {
		map[e.file_id] = e;
	}

	return map;
}

// Write database file back.
void write_db(const janus::config& config, const std::unordered_map<uint64_t, db_entry>& map)
{
	std::vector<db_entry> entries;
	for (auto [_, e] : map) {
		entries.push_back(e);
	}

	std::sort(entries.begin(), entries.end(),
		  [](db_entry& e1, db_entry& e2) { return e1.file_id < e2.file_id; });

	std::string path = config.binary_data_root + "/neptune.db";
	auto file = std::ofstream(path, std::ios::binary);
	if (!file)
		throw std::runtime_error(std::string("Can't open DB ") + path + " for writing.");

	file.write(reinterpret_cast<char*>(entries.data()), entries.size() * sizeof(db_entry));
}

// Write metadata file.
void write_meta(const std::string& dest_dir, janus::dynamic_buffer& dyn_buf)
{
	auto& header = dyn_buf.read<janus::meta_header>();
	uint64_t market_id = header.market_id;

	std::string path = dest_dir + "/" + std::to_string(market_id) + ".jan";

	auto file = std::ofstream(path, std::ios::binary);
	if (!file)
		throw std::runtime_error(std::string("Can't open ") + path + " for writing.");

	file.write(reinterpret_cast<char*>(dyn_buf.data()), dyn_buf.size());
}

// Parse metadata line from aggregated metadata file. Returns number of markets parsed.
auto parse_meta_line(const std::string& dest_dir, std::string& line) -> uint64_t
{
	sajson::document doc =
		janus::internal::parse_json("", reinterpret_cast<char*>(line.data()), line.size());
	const sajson::value& root = doc.get_root();

	if (root.get_type() != sajson::TYPE_ARRAY)
		throw std::runtime_error("Unexpected metadata root JSON type?");

	janus::dynamic_buffer dyn_buf(MAX_METADATA_BYTES);

	uint64_t count = root.get_length();
	for (uint64_t i = 0; i < count; i++) {
		sajson::value el = root.get_array_element(i);

		janus::betfair::parse_meta_json(el, dyn_buf);
		write_meta(dest_dir, dyn_buf);
		dyn_buf.reset();
	}

	return count;
}

// Parse metadata and save binary representation.
void parse_meta(janus::config& config, uint64_t file_id)
{
	std::string source_path =
		config.json_data_root + "/meta/" + std::to_string(file_id) + ".json";
	if (!file_exists(source_path))
		throw std::runtime_error(std::string("Cannot find metadata JSON file ") +
					 source_path);

	spdlog::info("Found new metadata file {}", source_path);

	std::string dest_dir = config.binary_data_root + "/meta/";
	if (!file_exists(dest_dir))
		throw std::runtime_error(std::string("Cannot find destination directory ") +
					 dest_dir);

	auto file = std::ifstream(source_path);
	if (!file)
		throw std::runtime_error(std::string("Cannot open file ") + source_path);

	spdlog::info("Writing metadata...");
	uint64_t num_markets = 0;
	std::string line;
	while (std::getline(file, line)) {
		if (line.empty() || line[0] == '\0')
			continue;

		num_markets += parse_meta_line(dest_dir, line);
	}
	spdlog::info("Wrote {} markets.", num_markets);
}

// Run core program loop.
auto run_loop(janus::config& config) -> bool
{
	spdlog::info("Reading DB file...");
	auto map = read_db(config);

	auto file_ids = get_file_id_list(config);
	spdlog::info("Found {} metadata files.", file_ids.size());
	spdlog::info("Checking for new metadata...");
	// Add entries for new files.
	for (uint64_t file_id : file_ids) {
		if (!map.contains(file_id)) {
			// If we haven't seen the file before, parse the
			// metadata and save it to the binary metadata
			// directory.
			parse_meta(config, file_id);
			map.emplace(file_id, file_id);
		}
	}

	spdlog::info("Writing DB file...");
	write_db(config, map);

	spdlog::info("Done!");
	return true;
}

auto main() -> int
{
	janus::config config = janus::parse_config();

	bool success;
	try {
		success = run_loop(config);
	} catch (std::exception& e) {
		spdlog::error(e.what());
		spdlog::critical("Aborting!");
		success = false;
	}

	return success ? 0 : 1;
}
