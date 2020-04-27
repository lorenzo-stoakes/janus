#include "janus.hh"

#include "sajson.hh"
#include "spdlog/spdlog.h"
#include <atomic>
#include <chrono>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <signal.h>
#include <string>
#include <vector>

// Show status line indicating number of lines processed.
#define SHOW_STATUS_LINE

// This is a hacked up skeleton version for testing purposes, initially we are simply logging in to
// betfair and performing simple queries.

// make the decltype slightly easier to the eye
using ms_t = std::chrono::milliseconds;

// Maximum number of subsequent stream errors before we abort.
static constexpr uint64_t MAX_NUM_STREAM_ERRORS = 10;

// How many lines before we flush output.
static constexpr uint64_t FLUSH_INTERVAL_LINES = 100;

// Retrieve ms since epoch.
static auto get_ms_since_epoch() -> decltype(ms_t().count())
{
	const auto now = std::chrono::system_clock::now();
	const auto epoch = now.time_since_epoch();
	const auto ms = std::chrono::duration_cast<ms_t>(epoch);

	return ms.count();
}

// Handle signals such that we can log out on e.g. ctrl+C.
// Courtesy of https://stackoverflow.com/a/4250601
std::atomic<bool> interrupted{false};
static void handle_signal(int)
{
	interrupted.store(true);
}
static void add_signal_handler()
{
	struct sigaction sa;
	memset(&sa, 0, sizeof(sa));
	sa.sa_handler = handle_signal;
	::sigfillset(&sa.sa_mask);
	::sigaction(SIGINT, &sa, nullptr);
}

// Setup directory structure if doesn't already exist.
static void setup_dirs(const std::string& meta_dir, const std::string& stream_dir)
{
	std::filesystem::create_directories(meta_dir);
	std::filesystem::create_directories(stream_dir);
}

// Save metadata to specified path.
static void save_meta(const std::string& meta_path, const std::string& meta)
{
	if (auto file = std::ofstream(meta_path)) {
		file << meta;
	} else {
		throw std::runtime_error(std::string("Cannot open ") + meta_path);
	}
}

// Write line of stream data to output.
static void write_stream_line(std::ofstream& file, const char* line, int size, uint64_t num_lines)
{
	file.write(line, size);
	file.put('\n');

	if (num_lines % FLUSH_INTERVAL_LINES == 0)
		file.flush();
}

#ifdef SHOW_STATUS_LINE
static void clear_line()
{
	std::cerr << "\r";
	for (int i = 0; i < 100; i++) // NOLINT: Not magical.
		std::cerr << " ";
	std::cerr << "\r" << std::flush;
}

static void print_status_line(uint64_t num_lines)
{
	clear_line();
	std::cerr << num_lines << " lines" << std::flush;
}
#endif

auto main(int argc, char** argv) -> int
{
	add_signal_handler();

	janus::config config = janus::parse_config();

	std::string meta_dir = config.json_data_root + "/meta/";
	std::string stream_dir = config.json_data_root + "/market_stream/";
	setup_dirs(meta_dir, stream_dir);

	std::string filename = std::to_string(get_ms_since_epoch()) + ".json";
	std::string meta_path = meta_dir + filename;
	std::string stream_path = stream_dir + filename;

	janus::tls::rng rng;
	rng.seed();
	janus::betfair::session session(rng, config);
	session.load_certs();

	spdlog::info("Logging in...");
	session.login();

	janus::betfair::stream stream(session);

	spdlog::info("Getting metadata...");
	uint64_t num_markets;
	std::string meta = stream.market_subscribe(config, num_markets);
	spdlog::info("Subscribed to {} markets", num_markets);
	spdlog::info("Saving metadata to {}...", meta_path);
	try {
		save_meta(meta_path, meta);
	} catch (std::exception& e) {
		spdlog::error(e.what());
		spdlog::critical("Cannot save metadata file, aborting...");
		return 1;
	}

	spdlog::info("Creating stream output file {}...", stream_path);
	auto stream_file = std::ofstream(stream_path);
	if (!stream_file) {
		spdlog::critical("Cannot open stream file {} for writing, aborting...",
				 stream_path);
		return 1;
	}

	spdlog::info("Starting stream...");

	uint64_t num_lines = 0;
	uint64_t num_errors = 0;
	while (true) {
		if (interrupted.load()) {
			spdlog::info("Signal received, aborting...");
			break;
		}

		try {
			int size;
			const char* line = stream.read_next_line(size);
			num_lines++;
			write_stream_line(stream_file, line, size, num_lines);
#ifdef SHOW_STATUS_LINE
			print_status_line(num_lines);
#endif
		} catch (std::exception& e) {
			spdlog::error(e.what());
			if (num_errors >= MAX_NUM_STREAM_ERRORS) {
				spdlog::critical("{} errors, cap is {}, aborting...", num_errors,
						 MAX_NUM_STREAM_ERRORS);
				return 1;
			}
		}
	}

	spdlog::info("Logging out...");
	session.logout();

	return 0;
}
