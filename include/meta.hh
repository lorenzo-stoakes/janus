#pragma once

#include <cstdint>

namespace janus
{
struct meta_header
{
	uint64_t market_id;
	uint64_t event_type_id;
	uint64_t event_id;
	uint64_t competition_id;
	uint64_t market_start_timestamp;
	uint64_t num_runners;
};
} // namespace janus
