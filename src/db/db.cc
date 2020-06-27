#include "janus.hh"

#include <algorithm>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <snappy.h>
#include <stdexcept>
#include <string>
#include <vector>
namespace fs = std::filesystem;

namespace janus
{
static auto get_id_list(const std::string& path, bool uncompressed_only = false)
	-> std::vector<uint64_t>
{
	std::vector<uint64_t> ret;

	for (const auto& entry : fs::directory_iterator(path)) {
		if (uncompressed_only && entry.path().extension() == ".snap")
			continue;

		std::string num_str = entry.path().stem().string();
		ret.push_back(std::stoll(num_str));
	}

	// In the case of JSON - since these are formated as ms since epoch,
	// sorting will give us the files in correct chronological order. In the
	// case of market data, this will be in market ID order.
	std::sort(ret.begin(), ret.end());
	return ret;
}

auto get_json_file_id_list(const janus::config& config) -> std::vector<uint64_t>
{
	std::string path = config.json_data_root + "/meta";
	return get_id_list(path);
}

auto get_meta_market_id_list(const janus::config& config) -> std::vector<uint64_t>
{
	std::string path = config.binary_data_root + "/meta";
	return get_id_list(path);
}

auto get_market_id_list(const janus::config& config) -> std::vector<uint64_t>
{
	std::string path = config.binary_data_root + "/market";
	return get_id_list(path);
}

auto get_uncompressed_market_id_list(const janus::config& config) -> std::vector<uint64_t>
{
	std::string path = config.binary_data_root + "/market";
	return get_id_list(path, true);
}

auto read_metadata(const config& config, dynamic_buffer& dyn_buf, uint64_t id) -> meta_view
{
	std::string path = config.binary_data_root + "/meta/" + std::to_string(id) + ".jan";

	auto file = std::ifstream(path, std::ios::binary | std::ios::ate);
	if (!file)
		throw std::runtime_error(std::string("Cannot open ") + path + " for metadata read");

	uint64_t size = file.tellg();
	file.seekg(0);

	char* ptr = static_cast<char*>(dyn_buf.reserve(size));
	if (!file.read(ptr, size))
		throw std::runtime_error(std::string("Error reading metadata from ") + path);

	return meta_view(dyn_buf);
}

auto read_all_metadata(const config& config, dynamic_buffer& dyn_buf) -> std::vector<meta_view>
{
	const auto ids = get_meta_market_id_list(config);
	std::vector<meta_view> ret;
	ret.reserve(ids.size());
	for (uint64_t id : ids) {
		// cppcheck-suppress useStlAlgorithm
		ret.emplace_back(read_metadata(config, dyn_buf, id));
	}

	// Sort chronologically by start time.
	std::sort(ret.begin(), ret.end(), [](meta_view v1, meta_view v2) {
		return v1.market_start_timestamp() < v2.market_start_timestamp();
	});
	return ret;
}

auto read_market_updates_string(const config& config, uint64_t id) -> std::string
{
	std::string path = config.binary_data_root + "/market/" + std::to_string(id) + ".jan";

	bool is_compressed = false;
	auto file = std::ifstream(path, std::ios::binary | std::ios::ate);
	if (!file) {
		path += ".snap";
		file = std::ifstream(path, std::ios::binary | std::ios::ate);
		if (!file)
			throw std::runtime_error(std::string("couldn't open ") + path +
						 " for reading");
		is_compressed = true;
	}

	uint64_t size = file.tellg();
	file.seekg(0);

	std::string str(size, '\0');
	if (!file.read(&str[0], size))
		throw std::runtime_error(std::string("Error reading market updates from ") + path);

	// Simple case - just read the data and put it in the dynamic buffer.
	if (!is_compressed)
		return str;

	std::string uncompressed;
	if (!snappy::Uncompress(&str[0], str.size(), &uncompressed))
		throw std::runtime_error(std::string("Unable to decompress ") + path +
					 " file corrupted?");

	return uncompressed;
}

auto read_market_updates(const config& config, dynamic_buffer& dyn_buf, uint64_t id) -> uint64_t
{
	std::string str = read_market_updates_string(config, id);
	dyn_buf.add_raw(str.c_str(), str.size());
	return str.size() / sizeof(janus::update);
}

auto read_market_updates(const config& config, uint64_t id) -> dynamic_buffer
{
	std::string str = read_market_updates_string(config, id);
	dynamic_buffer ret(str.size());
	ret.add_raw(str.c_str(), str.size());
	return ret;
}

auto index_market_updates(dynamic_buffer& dyn_buf, uint64_t num_updates) -> std::vector<uint64_t>
{
	std::vector<uint64_t> ret;

	// The read offset might be non-zero, ensure it is non-zero.
	dyn_buf.reset_read();

	for (uint64_t i = 0; i < num_updates; i++) {
		auto& u = dyn_buf.read<update>();

		if (u.type == update_type::TIMESTAMP)
			ret.push_back(i);
	}

	// Reset before we return so future use isn't offset.
	dyn_buf.reset_read();
	return ret;
}

auto have_market_stats(const config& config, uint64_t id) -> bool
{
	std::string path = config.binary_data_root + "/stats/" + std::to_string(id) + ".jan";
	return file_exists(path);
}

auto read_market_stats(const config& config, uint64_t id) -> stats
{
	std::string path = config.binary_data_root + "/stats/" + std::to_string(id) + ".jan";

	auto file = std::ifstream(path, std::ios::binary | std::ios::ate);
	if (!file)
		throw std::runtime_error(std::string("Cannot open ") + path + " for stats read");

	uint64_t size = file.tellg();
	file.seekg(0);
	if (size != sizeof(stats))
		throw std::runtime_error(std::string("Stats file ") + path +
					 " not of expected size " + std::to_string(sizeof(stats)) +
					 " is of size " + std::to_string(size));

	stats ret;
	if (!file.read(reinterpret_cast<char*>(&ret), size))
		throw std::runtime_error(std::string("Error while reading stats file ") + path);

	return ret;
}
} // namespace janus
