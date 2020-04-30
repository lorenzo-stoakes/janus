#include "janus.hh"

#include <algorithm>
#include <cstdint>
#include <filesystem>
#include <snappy.h>
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
} // namespace janus
