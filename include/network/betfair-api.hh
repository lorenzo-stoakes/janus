#pragma once

#include <string>
#include <vector>

#include "network/betfair-session.hh"

namespace janus::betfair
{
// Retrieve market IDs using the specified market filter JSON, returned in date
// order. If the results exceed 1000, then only the first 1000 are returned.
// https://docs.developer.betfair.com/display/1smk3cen4v3lu3yomq5qye0ni/Betting+Type+Definitions#BettingTypeDefinitions-MarketFilter
auto get_market_ids(session& session, std::string filter_json) -> std::vector<std::string>;
} // namespace janus::betfair
