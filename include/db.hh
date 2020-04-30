#pragma once

#include "config.hh"
#include "meta.hh"

#include <cstdint>
#include <vector>

namespace janus
{
// Retrieve list of JSON files. By convention the filename is [ms since
// epoch].json both for metadata and stream data.
auto get_json_file_id_list(const janus::config& config) -> std::vector<uint64_t>;

// Retrieve list of binary meta market IDs. By convention the filename is [ms since
// epoch].json both for metadata and stream data.
auto get_meta_market_id_list(const janus::config& config) -> std::vector<uint64_t>;
} // namespace janus
