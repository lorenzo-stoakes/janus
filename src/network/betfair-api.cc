#include "janus.hh"

#include <stdexcept>
#include <string>
#include <vector>

namespace janus::betfair
{
auto get_market_ids(session& session, const std::string& filter_json) -> std::vector<std::string>
{
	std::string json = R"({"sort":"FIRST_TO_START","maxResults":1000,"filter":)";
	json += filter_json + "}";

	std::string response = session.api("listMarketCatalogue", json);
	sajson::document doc = janus::internal::parse_json("", const_cast<char*>(response.data()),
							   response.size());
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

		ret.push_back(market_id.as_cstring());
	}
	return ret;
}
} // namespace janus::betfair
