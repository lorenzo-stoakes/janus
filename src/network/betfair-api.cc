#include "janus.hh"

#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace janus::betfair
{
auto get_meta(session& session, std::vector<std::string> market_ids)
{
	if (market_ids.empty())
		throw std::runtime_error("Empty market ID array in get_meta()?");

	// The data weighting system requires that we have to split out metadata retrieval into
	// blocks of 100 at a time.
	std::string prefix =
		R"({"sort":"FIRST_TO_START","maxResults":)" +
		std::to_string(MAX_METADATA_REQUESTS) +
		R"(,"marketProjection":["COMPETITION","EVENT","EVENT_TYPE","MARKET_START_TIME","MARKET_DESCRIPTION","RUNNER_DESCRIPTION","RUNNER_METADATA"],"filter":{"marketIds":[)";

	std::string meta;
	;
	for (uint64_t i = 0; i < market_ids.size(); i += MAX_METADATA_REQUESTS) {
		std::string json = prefix;
		for (uint64_t j = i; j < market_ids.size() && j < i + MAX_METADATA_REQUESTS; j++) {
			json += "\"" + market_ids[j] + "\",";
		}
		// Remove final comma.
		json.erase(json.size() - 1);
		json += "]}}";

		std::string response = session.api("listMarketCatalogue", json);
		// A newline should be appended to the end of the data delineated each block of
		// results.
		meta += response + "\n";
	}

	return meta;
}

auto get_market_ids(session& session, const std::string& filter_json)
	-> std::pair<std::vector<std::string>, std::string>
{
	std::string json = R"({"sort":"FIRST_TO_START","maxResults":1000,"filter":)";
	json += filter_json + "}";

	std::string response = session.api("listMarketCatalogue", json);

	sajson::document doc = janus::internal::parse_json("", &response[0], response.size());
	const sajson::value& root = doc.get_root();
	if (root.get_type() != sajson::TYPE_ARRAY)
		throw std::runtime_error("listMarketCatalogue returned non-array response");

	std::vector<std::string> ret;
	uint64_t count = root.get_length();
	for (uint64_t i = 0; i < count; i++) {
		sajson::value el = root.get_array_element(i);

		sajson::value market_id = el.get_value_of_key(sajson::literal("marketId"));
		if (market_id.get_type() != sajson::TYPE_STRING)
			throw std::runtime_error(
				"Unable to determine market ID for listMarketCatalogue element");

		ret.emplace_back(market_id.as_cstring());
	}

	std::string meta = get_meta(session, ret);
	return std::make_pair(ret, meta);
}
} // namespace janus::betfair
