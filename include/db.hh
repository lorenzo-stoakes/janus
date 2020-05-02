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

// Read metadata for specific market ID from binary file.
auto read_metadata(const config& config, dynamic_buffer& dyn_buf, uint64_t id) -> meta_view;

// Read all metadata from binary file directory and return collection of
// metadata views.
auto read_all_metadata(const config& config, dynamic_buffer& dyn_buf) -> std::vector<meta_view>;

// Read all market updates for the specified market ID and places in dynamic
// buffer. Transparently handles snappy-compressed data. Returns number of
// updates retrieved.
auto read_market_updates(const config& config, dynamic_buffer& dyn_buf, uint64_t id) -> uint64_t;

// Returns a vector of the indexes within the buffer which are timestamp
// updates. This can be used to correctly delineate between blocks of
// updates. The dynamic buffer has its read offset reset before returning.
auto index_market_updates(dynamic_buffer& dyn_buf) -> std::vector<uint64_t>;
} // namespace janus
