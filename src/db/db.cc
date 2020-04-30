#include "janus.hh"

#include <algorithm>
#include <cstdint>
#include <filesystem>
#include <snappy.h>
#include <vector>
namespace fs = std::filesystem;

namespace janus
{
auto get_json_file_id_list(const janus::config& config) -> std::vector<uint64_t>
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
} // namespace janus
