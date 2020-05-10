#include "janus.hh"

#include "sajson.hh"
#include "spdlog/spdlog.h"
#include <algorithm>
#include <atomic>
#include <cerrno>
#include <chrono>
#include <csignal>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <snappy.h>
#include <sstream>
#include <streambuf>
#include <string>
#include <sys/stat.h>
#include <thread>
#include <vector>
namespace fs = std::filesystem;

static constexpr uint64_t MAX_METADATA_BYTES = 100'000;
static constexpr uint64_t MAX_STREAM_BYTES = 1'000'000'000;
static constexpr uint64_t MAX_LEGACY_STREAM_BYTES = 10'000'000'000;
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

	// We intentionally make these fields public :) I'm a C programmer at heart...
	uint64_t file_id;      // NOLINT
	uint64_t last_updated; // NOLINT
	uint64_t next_offset;  // NOLINT
	uint64_t next_line;    // NOLINT
};

using db_t = std::unordered_map<uint64_t, db_entry>;
using per_market_t = std::unordered_map<uint64_t, std::vector<janus::update>>;

// Handle SIGINT, e.g. ctrl+C.
static void handle_interrupt(int) // NOLINT: Unused but required param.
{
	signalled.store(true);
}

// Add signal handlers.
static void add_signal_handler()
{
	struct sigaction action_interrupt = {{nullptr}};
	action_interrupt.sa_handler = handle_interrupt;
	::sigfillset(&action_interrupt.sa_mask);
	::sigaction(SIGINT, &action_interrupt, nullptr);
}

// Does the file at the specified path exist?
static auto file_exists(const std::string& path) -> bool
{
	std::ifstream f(path);
	return f.good();
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
		if (!file.read(reinterpret_cast<char*>(entries.data()), size))
			throw std::runtime_error(std::string("Error reading ") + path);
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

// Read all legacy metadata JSON files, parse them and write binary output.
void parse_all_legacy_meta(const janus::config& config)
{
	std::string legacy_root_dir = config.json_data_root + "/legacy/markets/";
	if (!file_exists(legacy_root_dir))
		throw std::runtime_error(std::string("Cannot find legacy market directory ") +
					 legacy_root_dir);

	std::string dest_dir = config.binary_data_root + "/meta/";
	if (!file_exists(dest_dir))
		throw std::runtime_error(std::string("Cannot find destination directory ") +
					 dest_dir);

	uint64_t num_markets = 0;
	janus::dynamic_buffer dyn_buf(MAX_METADATA_BYTES);
	for (const auto& entry : fs::directory_iterator(legacy_root_dir)) {
		std::string path = entry.path().string() + "/meta.json";

		auto file = std::ifstream(path);
		if (!file)
			throw std::runtime_error(std::string("Cannot open file ") + path);
		std::string json((std::istreambuf_iterator<char>(file)),
				 std::istreambuf_iterator<char>());

		janus::betfair::parse_meta_json(path.c_str(), &json[0], json.size(), dyn_buf);
		write_meta(dest_dir, dyn_buf);

		dyn_buf.reset();
		num_markets++;
	}

	spdlog::info("Metadata generated for {} legacy markets", num_markets);
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

	struct stat sb = {0};
	int err_code = stat(path.c_str(), &sb);
	if (err_code != 0) {
		std::string err = ::strerror(errno);
		throw std::runtime_error(std::string("Cannot stat ") + path + ": " + err);
	}

	auto last_access = static_cast<uint64_t>(sb.st_mtime);
	auto size = static_cast<uint64_t>(sb.st_size);
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
auto extract_by_market(std::string path, janus::dynamic_buffer& dyn_buf, uint64_t num_updates)
	-> per_market_t
{
	std::unordered_map<uint64_t, std::vector<janus::update>> map;

	if (num_updates < 2) {
		spdlog::warn("Only {} received from {} - expected minimum 2, ignoring.",
			     num_updates, path);
		return map;
	}

	// Read first update, HAS to be a timestamp.
	auto& first = dyn_buf.read<janus::update>();
	if (first.type != janus::update_type::TIMESTAMP) {
		std::ostringstream oss;
		oss << path << ": First update is of type " << janus::update_type_str(first.type)
		    << " not expected timestamp.";
		throw std::runtime_error(oss.str());
	}
	uint64_t timestamp = janus::get_update_timestamp(first);

	// Read second update, HAS to be a timestamp.
	auto& second = dyn_buf.read<janus::update>();
	if (second.type != janus::update_type::MARKET_ID) {
		std::ostringstream oss;

		oss << path << ": Second update is of type " << janus::update_type_str(second.type)
		    << " not expected market ID.";
		throw std::runtime_error(oss.str());
	}
	uint64_t market_id = janus::get_update_market_id(second);

	for (uint64_t i = 2; i < num_updates; i++) {
		auto& u = dyn_buf.read<janus::update>();
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
			      const std::vector<janus::update>& updates, bool remove_snap) -> bool
{
	std::string path = destdir + std::to_string(id) + ".jan";
	if (remove_snap) {
		// Succeeds even if file doesn't exist.
		fs::remove(path + ".snap");
	} else if (file_exists(path + ".snap")) {
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
void write_stream_data(const janus::config& config, const per_market_t& per_market,
		       bool remove_snap = false)
{
	if (per_market.size() == 0)
		return;

	std::string destdir = config.binary_data_root + "/market/";

	uint64_t markets_written = 0;
	for (const auto& [id, updates] : per_market) {
		if (!write_market_stream_data(destdir, id, updates, remove_snap)) {
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

	auto per_market = extract_by_market(source, dyn_buf, num_updates);
	spdlog::debug("Extracted data for {} markets.", per_market.size());

	spdlog::debug("Writing market data...");
	write_stream_data(config, per_market);
	entry.next_offset = new_offset;
	entry.next_line = state.line;
}

// Parse all legacy JSON stream files and converts to binary. Returns false if signal interrupted.
// TODO(lorenzo): Duplication between this function and update_from_stream_file().
auto parse_all_legacy_stream(const janus::config& config) -> bool
{
	std::string legacy_root_dir = config.json_data_root + "/legacy/all/";
	if (!file_exists(legacy_root_dir))
		throw std::runtime_error(std::string("Cannot find legacy all streams directory ") +
					 legacy_root_dir);

	janus::dynamic_buffer dyn_buf(MAX_LEGACY_STREAM_BYTES);
	janus::betfair::price_range range;

	uint64_t num_files = 0;
	for (const auto& entry : fs::directory_iterator(legacy_root_dir)) {
		if (signalled.load()) {
			spdlog::info("Signal received, aborting...");
			return false;
		}

		std::string source = entry.path().string();

		auto file = std::ifstream(source, std::ios::ate);
		if (!file)
			throw std::runtime_error(std::string("Cannot open ") + source +
						 " for stream update read");
		uint64_t bytes = file.tellg();
		file.seekg(0);

		// We log at INFO level since this is an irregular operation and
		// slow, so we will want to see progress.
		spdlog::info("Reading from {} of size {} bytes...", source, bytes);

		janus::betfair::update_state state = {
			.range = &range,
			.filename = source.c_str(),
			.line = 1,
		};

		uint64_t num_lines = 0;
		uint64_t num_updates = 0;
		std::string line;
		while (std::getline(file, line)) {
			if (line.empty() || line[0] == '\0') {
				state.line++;
				continue;
			}

			num_updates += janus::betfair::parse_update_stream_json(
				state, &line[0], line.size(), dyn_buf);
			num_lines++;
		}
		spdlog::debug("Read {} lines ({} bytes) from {} resulting in {} updates.",
			      num_lines, bytes, source, num_updates);

		spdlog::debug("Extracting per-market data...");
		auto per_market = extract_by_market(source, dyn_buf, num_updates);
		spdlog::debug("Extracted data for {} markets.", per_market.size());

		spdlog::debug("Writing market data, deleting any existing .snap files...");
		write_stream_data(config, per_market, true);

		dyn_buf.reset();
		num_files++;
	}

	spdlog::info("Read {} legacy stream JSON files.", num_files);
	return true;
}

// Determine whether market data in specified dynamic buffer contains a
// MARKET_CLOSE update.
auto market_closes(janus::dynamic_buffer& dyn_buf) -> bool
{
	while (dyn_buf.read_offset() != dyn_buf.size()) {
		auto& u = dyn_buf.read<janus::update>();
		if (u.type == janus::update_type::MARKET_CLOSE)
			return true;
	}

	return false;
}

// Snappy-compress specified file, outputting as [path].snap, deleting the
// existing file afterwards.
void snappify(std::string path, janus::dynamic_buffer& dyn_buf)
{
	std::string dest = path + ".snap";

	std::string compressed;
	snappy::Compress(reinterpret_cast<const char*>(dyn_buf.data()), dyn_buf.size(),
			 &compressed);

	if (auto file = std::ofstream(dest, std::ios::binary)) {
		file.write(compressed.c_str(), compressed.size());
	} else {
		throw std::runtime_error(std::string("Unable to snappy compress ") + path +
					 " cannot open " + dest + " for write.");
	}
	fs::remove(path);

	spdlog::debug("Compressed {} of size {} to {} bytes at {}", path, dyn_buf.size(),
		      compressed.size(), dest);
}

// 'Snappify' i.e. snappy-compress markets that have seen a MARKET_CLOSE update,
// removing the existing file and outputting the compressed file with an
// appended .snap extension. Returns false if signalled to stop.
auto snappify_markets(const janus::config& config) -> bool
{
	std::string market_dir = config.binary_data_root + "/market/";
	if (!file_exists(market_dir))
		throw std::runtime_error(std::string("Cannot find binary market directory ") +
					 market_dir);

	janus::dynamic_buffer dyn_buf(MAX_STREAM_BYTES);

	uint64_t num_markets = 0;
	uint64_t num_snappified = 0;
	for (const auto& entry : fs::directory_iterator(market_dir)) {
		if (signalled.load()) {
			spdlog::info("Signal received, aborting...");
			return false;
		}

		if (entry.path().extension() != ".jan")
			continue;

		std::string source = entry.path().string();

		auto file = std::ifstream(source, std::ios::binary | std::ios::ate);
		if (!file)
			throw std::runtime_error(std::string("Cannot open ") + source +
						 " for stream data read");

		uint64_t size = file.tellg();
		file.seekg(0);

		char* ptr = static_cast<char*>(dyn_buf.reserve(size));
		if (!file.read(ptr, size))
			throw std::runtime_error(std::string("Error reading stream data from ") +
						 source);

		try {
			if (market_closes(dyn_buf)) {
				snappify(source, dyn_buf);
				num_snappified++;
			}
		} catch (std::runtime_error& e) {
			spdlog::error("Error processing {}!", source);
			throw;
		}

		dyn_buf.reset();
		num_markets++;
	}

	spdlog::info("Snappified {} of {} markets.", num_snappified, num_markets);
	return true;
}

// Run core functionality.
auto run_core(const janus::config& config, bool force_meta) -> bool
{
	spdlog::debug("Reading DB file...");
	auto db = read_db(config);

	auto file_ids = janus::get_json_file_id_list(config);
	spdlog::debug("Found {} metadata files.", file_ids.size());

	if (force_meta) {
		spdlog::debug("force-meta mode: parsing all {} metadata files...", file_ids.size());

		for (uint64_t file_id : file_ids) {
			parse_meta(config, file_id);
			db.emplace(file_id, file_id);
		}
	} else {
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
	}

	auto update_ids = get_update_id_list(config, db);
	if (!update_ids.empty()) {
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
auto run_loop(const janus::config& config, bool force_meta) -> bool
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
			if (!run_core(config, force_meta))
				return false;
			// We only force ALL metadata retrieval on the FIRST
			// iteration.
			force_meta = false;
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

void usage(std::string cmd)
{
	spdlog::info("usage:");
	spdlog::info("{} [flags...]", cmd);
	spdlog::info("  --help                - Display this message.");
	spdlog::info("  --force-legacy-meta   - Force regeneration of legacy metadata.");
	spdlog::info(
		"  --force-meta          - Force regeneration of all metadata including legacy.");
	spdlog::info(
		"  --force-legacy-stream - Force regeneration of all legacy market stream data and snappify.");
	spdlog::info("  --snappify            - Compress closed markets.");
}

// Read command-line flags. Returns false to exit immediately.
auto read_flags(int argc, char** argv, bool& force_legacy_meta, bool& force_meta,
		bool& force_legacy_stream, bool& snappify) -> bool
{
	for (int i = 1; i < argc; i++) {
		std::string arg = argv[i];

		if (arg == "--help") {
			usage(argv[0]);
			return false;
		} else if (arg == "--force-legacy-meta") {
			spdlog::info("Forcing full refresh of legacy metadata!");
			force_legacy_meta = true;
		} else if (arg == "--force-meta") {
			spdlog::info("Forcing full refresh of metadata!");
			force_meta = true;
			// If we are refreshing all metadata we will want legacy
			// metadata too.
			force_legacy_meta = true;
		} else if (arg == "--force-legacy-stream") {
			spdlog::info("Forcing full refresh of legacy stream data!");
			force_legacy_stream = true;
			// If we are retrieving legacy stream data we will want
			// to snappify it afterwards.
			snappify = true;
		} else if (arg == "--snappify") {
			spdlog::info("Will snappify markets which have seen market close update.");
			snappify = true;
		} else if (arg.starts_with("--")) {
			spdlog::error("Unrecognised flag '{}'", arg);
			usage(argv[0]);
			return false;
		} else {
			spdlog::error("Unrecognised parameter '{}'", arg);
			usage(argv[0]);
			return false;
		}
	}

	return true;
}

auto main(int argc, char** argv) -> int // NOLINT: Handles exceptions!
{
	try {
		add_signal_handler();

		spdlog::info("neptune " STR(GIT_VER));
		janus::config config = janus::parse_config();

		bool force_legacy_meta = false;
		bool force_meta = false;
		bool force_legacy_stream = false;
		bool snappify = false;
		if (!read_flags(argc, argv, force_legacy_meta, force_meta, force_legacy_stream,
				snappify))
			return 0;

		if (force_legacy_meta) {
			spdlog::info("Forced legacy meta update, parsing data...");
			parse_all_legacy_meta(config);
		}

		if (force_legacy_stream) {
			spdlog::info("Forced legacy stream update, this might take a while...");
			// Returns false if signal received, in that case just exit.
			if (!parse_all_legacy_stream(config))
				return 0;
		}

		if (snappify) {
			spdlog::info("Snappifying markets, this might take a while...");
			if (!snappify_markets(config))
				return 0;
		}

		return run_loop(config, force_meta) ? 0 : 1;
	} catch (std::exception& e) {
		spdlog::error(e.what());
		spdlog::critical("Aborting!");
	} catch (...) {
		spdlog::critical("Unknown exception raised, aborting!!");
	}

	return 1;
}
