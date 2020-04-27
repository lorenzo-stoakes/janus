#pragma once

#include <cstdint>
#include <string>
#include <utility>
#include <vector>

#include "network/betfair-session.hh"

namespace janus::betfair
{
// The maximum number of markets we can request at once. Betfair-imposed data limit.
static constexpr uint64_t MAX_METADATA_REQUESTS = 100;

// Retrieve market IDs using the specified market filter JSON, returned in date
// order. If the results exceed 1000, then only the first 1000 are returned.
// https://docs.developer.betfair.com/display/1smk3cen4v3lu3yomq5qye0ni/Betting+Type+Definitions#BettingTypeDefinitions-MarketFilter
auto get_market_ids(session& session, const std::string& filter_json)
	-> std::pair<std::vector<std::string>, std::string>;
} // namespace janus::betfair
