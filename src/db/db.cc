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
static auto get_id_list(const std::string& path) -> std::vector<uint64_t>
{
	std::vector<uint64_t> ret;

	for (const auto& entry : fs::directory_iterator(path)) {
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

	return ret;
}

auto read_market_updates(const config& config, dynamic_buffer& dyn_buf, uint64_t id) -> uint64_t
{
	std::string path = config.binary_data_root + "/market/" + std::to_string(id) + ".jan";

	bool is_compressed = false;
	auto file = std::ifstream(path, std::ios::binary | std::ios::ate);
	if (!file) {
		path += ".snap";
		file = std::ifstream(path, std::ios::binary | std::ios::ate);
		if (!file)
			throw std::runtime_error(std::string("couldn't open ") + path +
						 "[.snap] for reading");
		is_compressed = true;
	}

	uint64_t size = file.tellg();
	file.seekg(0);

	// Simple case - just read the data and put it in the dynamic buffer.
	if (!is_compressed) {
		char* ptr = static_cast<char*>(dyn_buf.reserve(size));
		if (!file.read(ptr, size))
			throw std::runtime_error(std::string("Error reading market updates from ") +
						 path);

		return size / sizeof(janus::update);
	}

	std::string compressed(size, '\0');
	if (!file.read(&compressed[0], size))
		throw std::runtime_error(
			std::string("Error reading compressed market updates from ") + path);

	std::string uncompressed;
	if (!snappy::Uncompress(compressed.c_str(), compressed.size(), &uncompressed))
		throw std::runtime_error(std::string("Unable to decompress ") + path +
					 " file corrupted?");
	dyn_buf.add_raw(uncompressed.c_str(), uncompressed.size());
	return uncompressed.size() / sizeof(janus::update);
}
} // namespace janus
