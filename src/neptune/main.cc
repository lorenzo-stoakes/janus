#include "janus.hh"

#include "sajson.hh"
#include "spdlog/spdlog.h"
#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstdint>
#include <cstring>
#include <errno.h>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <signal.h>
#include <sstream>
#include <string>
#include <sys/stat.h>
#include <thread>
#include <vector>

namespace fs = std::filesystem;

static constexpr uint64_t MAX_METADATA_BYTES = 100'000;
static constexpr uint64_t MAX_STREAM_BYTES = 1'000'000'000;
static constexpr uint64_t SLEEP_INTERVAL_MS = 1000;

// Indicates whether a signal has occured and we should abort.
std::atomic<bool> signalled{false};

// Represents entry in neptune.db database file.
struct db_entry
{
	db_entry() : file_id{0}, last_updated{0}, next_offset{0}, next_line{0} {}
	explicit db_entry(uint64_t id) : file_id{id}, last_updated{0}, next_offset{0}, next_line{0}
	{
	}

	uint64_t file_id;
	uint64_t last_updated;
	uint64_t next_offset;
	uint64_t next_line;
};

using db_t = std::unordered_map<uint64_t, db_entry>;
using per_market_t = std::unordered_map<uint64_t, std::vector<janus::update>>;

// Handle SIGINT, e.g. ctrl+C.
static void handle_interrupt(int)
{
	signalled.store(true);
}

// Add signal handlers.
static void add_signal_handler()
{
	struct sigaction action_interrupt = {0};
	action_interrupt.sa_handler = handle_interrupt;
	::sigfillset(&action_interrupt.sa_mask);
	::sigaction(SIGINT, &action_interrupt, nullptr);
}

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
auto read_db(const janus::config& config) -> db_t
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
	db_t db;
	for (auto e : entries) {
		db[e.file_id] = e;
	}

	return db;
}

// Write database file back.
void write_db(const janus::config& config, const db_t& db)
{
	std::vector<db_entry> entries;
	for (const auto& [_, e] : db) {
		entries.push_back(e);
	}

	std::sort(entries.begin(), entries.end(),
		  [](const db_entry& e1, const db_entry& e2) { return e1.file_id < e2.file_id; });

	std::string path = config.binary_data_root + "/neptune.db";
	auto file = std::ofstream(path, std::ios::binary);
	if (!file)
		throw std::runtime_error(std::string("Can't open DB ") + path + " for writing.");

	file.write(reinterpret_cast<char*>(entries.data()), entries.size() * sizeof(db_entry));
}

// Write metadata file.
void write_meta(const std::string& dest_dir, janus::dynamic_buffer& dyn_buf)
{
	const auto& header = dyn_buf.read<janus::meta_header>();
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
void parse_meta(const janus::config& config, uint64_t file_id)
{
	std::string source_path =
		config.json_data_root + "/meta/" + std::to_string(file_id) + ".json";
	if (!file_exists(source_path))
		throw std::runtime_error(std::string("Cannot find metadata JSON file ") +
					 source_path);

	spdlog::debug("Found new metadata file {}", source_path);

	std::string dest_dir = config.binary_data_root + "/meta/";
	if (!file_exists(dest_dir))
		throw std::runtime_error(std::string("Cannot find destination directory ") +
					 dest_dir);

	auto file = std::ifstream(source_path);
	if (!file)
		throw std::runtime_error(std::string("Cannot open file ") + source_path);

	spdlog::debug("Writing metadata...");
	uint64_t num_markets = 0;
	std::string line;
	while (std::getline(file, line)) {
		if (line.empty() || line[0] == '\0')
			continue;

		num_markets += parse_meta_line(dest_dir, line);
	}
	spdlog::debug("Wrote {} markets.", num_markets);
}

// Get path for JSON stream data file.
auto get_stream_json_path(const janus::config& config, uint64_t id) -> std::string
{
	return config.json_data_root + "/market_stream/" + std::to_string(id) + ".json";
}

// Has the stream data file specified by the file ID changed since we last saw
// it?
auto check_stream_file_changed(const janus::config& config, uint64_t file_id, db_entry& entry)
	-> bool
{
	std::string path = get_stream_json_path(config, file_id);

	struct stat sb;
	int err_code = stat(path.c_str(), &sb);
	if (err_code != 0) {
		std::string err = ::strerror(errno);
		throw std::runtime_error(std::string("Cannot stat ") + path + ": " + err);
	}

	uint64_t last_access = static_cast<uint64_t>(sb.st_mtime);
	uint64_t size = static_cast<uint64_t>(sb.st_size);
	if (last_access > entry.last_updated && size > entry.next_offset) {
		// Update DB now, we will write back later.
		entry.last_updated = last_access;
		return true;
	}

	return false;
}

// Scan files in DB and check whether market stream files changed since. Updates
// DB to set new update time.
auto get_update_id_list(const janus::config& config, db_t& db) -> std::vector<uint64_t>
{
	std::vector<uint64_t> ret;

	for (auto& p : db) {
		if (check_stream_file_changed(config, p.first, p.second))
			ret.push_back(p.first);
	}

	return ret;
}

// Extract per-market data from the dynamic buffer which has accumulated delta
// stream updates.
auto extract_by_market(janus::dynamic_buffer& dyn_buf, uint64_t num_updates) -> per_market_t
{
	if (num_updates < 2) {
		std::ostringstream oss;
		oss << "Only " << num_updates << " updates, expected minimum 2?";
		throw std::runtime_error(oss.str());
	}

	// Read first update, HAS to be a timestamp.
	janus::update& first = dyn_buf.read<janus::update>();
	if (first.type != janus::update_type::TIMESTAMP) {
		std::ostringstream oss;
		oss << "First update is of type " << janus::update_type_str(first.type)
		    << " not expected timestamp.";
		throw std::runtime_error(oss.str());
	}
	uint64_t timestamp = janus::get_update_timestamp(first);

	// Read second update, HAS to be a timestamp.
	janus::update& second = dyn_buf.read<janus::update>();
	if (second.type != janus::update_type::MARKET_ID) {
		std::ostringstream oss;

		oss << "Second update is of type " << janus::update_type_str(second.type)
		    << " not expected market ID.";
		throw std::runtime_error(oss.str());
	}
	uint64_t market_id = janus::get_update_market_id(second);

	std::unordered_map<uint64_t, std::vector<janus::update>> map;

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

auto write_market_stream_data(const std::string& destdir, uint64_t id,
			      const std::vector<janus::update>& updates) -> bool
{
	std::string path = destdir + std::to_string(id) + ".jan";
	if (file_exists(path + ".snap")) {
		spdlog::warn(
			"Received {} updates for market {} but already archived as {}.snap? Skipping.",
			updates.size(), id, path);
		return true;
	}

	auto file = std::ofstream(path, std::ios::binary | std::ios::app);
	if (!file) {
		spdlog::error(
			"Unable to open {} for writing - DATA MIGHT BE IN INCONSISTENT STATE!!",
			path);

		return false;
	}

	file.write(reinterpret_cast<const char*>(updates.data()),
		   updates.size() * sizeof(janus::update));

	return true;
}

// Write per-market stream data to individual binary files,
void write_stream_data(const janus::config& config, const per_market_t& per_market)
{
	std::string destdir = config.binary_data_root + "/market/";

	uint64_t markets_written = 0;
	for (const auto& [id, updates] : per_market) {
		if (!write_market_stream_data(destdir, id, updates)) {
			// For information output each previously written market.
			uint64_t logged_markets = 0;
			for (const auto& p : per_market) {
				if (++logged_markets > markets_written)
					break;

				spdlog::warn("NOTE: Previously wrote {}", p.first);
			}
			throw std::runtime_error("Write error");
		}
		markets_written++;
	}

	spdlog::debug("Wrote {} markets.", markets_written);
}

// Parse new data from stream file with specified file ID, updating entry to
// reflect new offset, line after parse and dynamic buffer filled with data.
void update_from_stream_file(const janus::config& config, const janus::betfair::price_range& range,
			     uint64_t id, db_entry& entry, janus::dynamic_buffer& dyn_buf)
{
	std::string source = get_stream_json_path(config, id);

	auto file = std::ifstream(source, std::ios::ate);
	if (!file)
		throw std::runtime_error(std::string("Cannot open ") + source +
					 " for stream update read");

	uint64_t size = file.tellg();

	uint64_t offset = entry.next_offset;
	if (offset >= size) {
		spdlog::warn("File {} modified but offset {} >= size {}", source, offset, size);
	}
	file.seekg(offset);

	uint64_t line_num = entry.next_line;
	if (line_num == 0)
		line_num = 1;

	janus::betfair::update_state state = {
		.range = &range,
		.filename = source.c_str(),
		.line = line_num,
	};

	std::string line;
	uint64_t num_updates = 0;
	while (std::getline(file, line)) {
		if (line.empty() || line[0] == '\0') {
			state.line++;
			continue;
		}

		num_updates += janus::betfair::parse_update_stream_json(
			state, reinterpret_cast<char*>(line.data()), line.size(), dyn_buf);
	}

	// Need to clear EOF flags to obtain correct offset. Probably this will
	// be equal to size but as file might be written in meantime can't be
	// sure.
	file.clear();
	uint64_t new_offset = file.tellg();

	uint64_t bytes = new_offset - entry.next_offset;
	uint64_t num_lines = state.line - entry.next_line + 1;

	spdlog::debug("Read {} lines ({} bytes) from {} resulting in {} updates.", num_lines, bytes,
		      source, num_updates);

	spdlog::debug("Extracting per-market data...");

	auto per_market = extract_by_market(dyn_buf, num_updates);
	spdlog::debug("Extracted data for {} markets.", per_market.size());

	spdlog::debug("Writing market data...");
	write_stream_data(config, per_market);
	entry.next_offset = new_offset;
	entry.next_line = state.line;
}

// Run core functionality.
auto run_core(const janus::config& config) -> bool
{
	spdlog::debug("Reading DB file...");
	auto db = read_db(config);

	auto file_ids = get_file_id_list(config);
	spdlog::debug("Found {} metadata files.", file_ids.size());
	spdlog::debug("Checking for new metadata...");
	// Add entries for new files.
	for (uint64_t file_id : file_ids) {
		if (!db.contains(file_id)) {
			// If we haven't seen the file before, parse the
			// metadata and save it to the binary metadata
			// directory.
			parse_meta(config, file_id);
			db.emplace(file_id, file_id);
		}
	}

	auto update_ids = get_update_id_list(config, db);
	if (update_ids.size() > 0) {
		spdlog::debug("Found {} market stream files with new data.", update_ids.size());
		spdlog::debug("Updating stream data...");

		janus::dynamic_buffer dyn_buf(MAX_STREAM_BYTES);
		janus::betfair::price_range range;

		for (uint64_t id : update_ids) {
			update_from_stream_file(config, range, id, db[id], dyn_buf);
		}
	}

	spdlog::debug("Writing DB file...");
	write_db(config, db);

	spdlog::debug("Done!");
	return true;
}

// Run core loop.
auto run_loop(const janus::config& config) -> bool
{
	// On the first run, output debug info
	bool first = true;
	spdlog::set_level(spdlog::level::debug);

	while (true) {
		if (signalled.load()) {
			spdlog::info("Signal received, aborting...");
			return true;
		}

		try {
			if (!run_core(config))
				return false;
		} catch (std::exception& e) {
			spdlog::error(e.what());
			spdlog::critical("Aborting!");
			return false;
		}

		if (first) {
			first = false;
			spdlog::set_level(spdlog::level::info);
			spdlog::info("Switched to log level INFO");
		}

		// TODO(lorenzo): inotify implementation.
		std::this_thread::sleep_for(std::chrono::milliseconds(SLEEP_INTERVAL_MS));
	}
}

auto main() -> int
{
	add_signal_handler();

	spdlog::info("neptune " STR(GIT_VER));

	janus::config config = janus::parse_config();
	return run_loop(config) ? 0 : 1;
}
