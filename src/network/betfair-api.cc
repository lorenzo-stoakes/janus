#include "janus.hh"

#include <iomanip>
#include <sstream>
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

static auto gen_place_order_json(uint64_t market_id, bet& bet, const std::string& strategy_ref,
				 const std::string& order_ref, bool fill_or_kill) -> std::string
{
	// Generate query JSON
	// See https://docs.developer.betfair.com/display/1smk3cen4v3lu3yomq5qye0ni/placeOrders

	std::ostringstream oss;

	oss << R"({"marketId":"1.)" << market_id << "\"";
	if (!strategy_ref.empty())
		oss << R"(,"customerStrategyRef":")" << strategy_ref << "\"";
	oss << R"(,"instructions":[)";
	oss << R"({"orderType":"LIMIT","selectionId":)" << bet.runner_id() << ",";
	oss << R"("side":")";
	if (bet.is_back())
		oss << "BACK";
	else
		oss << "LAY";
	oss << "\"";
	if (!order_ref.empty())
		oss << R"(,"customerOrderRef":")" << order_ref << "\"";

	oss << R"(,"limitOrder":{)";
	oss << R"("size":)" << std::fixed << std::setprecision(2) << bet.unmatched(); // NOLINT
	oss << R"(,"price":)" << std::fixed << std::setprecision(2) << bet.price();   // NOLINT
	oss << R"(,"persistenceType":")";
	switch (bet.persist()) {
	case bet_persist_type::LAPSE:
		oss << "LAPSE";
		break;
	case bet_persist_type::PERSIST:
		oss << "PERSIST";
		break;
	case bet_persist_type::MARKET_ON_CLOSE:
		oss << "MARKET_ON_CLOSE";
		break;
	}
	oss << "\"";

	if (fill_or_kill)
		oss << R"(,"timeInForce":"FILL_OR_KILL")";

	oss << "}}]}";

	return oss.str();
}

static auto parse_limit_order(sajson::value node, const std::string& response) -> limit_order
{
	limit_order ret{};

	sajson::value size = node.get_value_of_key(sajson::literal("size"));
	if (size.get_type() == sajson::TYPE_NULL)
		throw std::runtime_error(std::string("Missing size in limit order: ") + response);
	ret.size = size.get_number_value();

	sajson::value price = node.get_value_of_key(sajson::literal("price"));
	if (price.get_type() == sajson::TYPE_NULL)
		throw std::runtime_error(std::string("Missing price in limit order: ") + response);
	ret.price = price.get_number_value();

	// We might not get a persistence type if the bet was fully matched.
	sajson::value persist = node.get_value_of_key(sajson::literal("persistenceType"));
	if (persist.get_type() == sajson::TYPE_STRING) {
		std::string persist_str = persist.as_cstring();
		if (persist_str == "LAPSE")
			ret.persist = bet_persist_type::LAPSE;
		else if (persist_str == "PERSIST")
			ret.persist = bet_persist_type::PERSIST;
		else if (persist_str == "MARKET_ON_CLOSE")
			ret.persist = bet_persist_type::MARKET_ON_CLOSE;
		else
			throw std::runtime_error(std::string("Unknown persistence type ") +
						 persist_str + ": " + response);
	}

	ret.fill_or_kill = false;
	sajson::value tif = node.get_value_of_key(sajson::literal("timeInForce"));
	if (tif.get_type() == sajson::TYPE_STRING) {
		std::string tif_str = tif.as_cstring();
		if (tif_str == "FILL_OR_KILL")
			ret.fill_or_kill = true;
		else
			throw std::runtime_error(std::string("Unrecognised time in force of ") +
						 tif_str + " :" + response);
	}

	return ret;
}

static auto parse_place_instruction(sajson::value node, const std::string& response)
	-> place_instruction
{
	place_instruction ret{};

	sajson::value order_type = node.get_value_of_key(sajson::literal("orderType"));
	if (order_type.get_type() == sajson::TYPE_STRING)
		ret.order_type = order_type.as_cstring();
	sajson::value selection_id = node.get_value_of_key(sajson::literal("selectionId"));
	if (selection_id.get_type() == sajson::TYPE_INTEGER)
		ret.selection_id = selection_id.get_integer_value();

	sajson::value side = node.get_value_of_key(sajson::literal("side"));
	if (side.get_type() != sajson::TYPE_STRING)
		throw std::runtime_error(std::string("Missing side in place instruction: ") +
					 response);
	std::string side_str = side.as_cstring();
	if (side_str == "BACK")
		ret.is_back = true;
	else if (side_str == "LAY")
		ret.is_back = false;
	else
		throw std::runtime_error(std::string("Unrecognised side value '") + side_str +
					 "': " + response);

	sajson::value customer_order_ref =
		node.get_value_of_key(sajson::literal("customerOrderRef"));
	if (customer_order_ref.get_type() == sajson::TYPE_STRING)
		ret.customer_order_ref = customer_order_ref.as_cstring();

	sajson::value limit_order = node.get_value_of_key(sajson::literal("limitOrder"));
	if (limit_order.get_type() != sajson::TYPE_OBJECT)
		throw std::runtime_error(std::string("Invalid limit order: ") + response);
	ret.order = parse_limit_order(limit_order, response);

	return ret;
}

static auto parse_place_instruction_report(sajson::value node, const std::string& response)
	-> place_instruction_report
{
	place_instruction_report ret{};

	std::string status = node.get_value_of_key(sajson::literal("status")).as_cstring();
	ret.status = status;

	if (status == "SUCCESS")
		ret.result = result_type::SUCCESS;
	else if (status == "FAILURE")
		ret.result = result_type::FAILURE;
	else
		ret.result = result_type::UNKNOWN;

	sajson::value err_code = node.get_value_of_key(sajson::literal("errorCode"));
	if (err_code.get_type() == sajson::TYPE_STRING)
		ret.error_code = err_code.as_cstring();

	sajson::value order_status = node.get_value_of_key(sajson::literal("orderStatus"));
	if (order_status.get_type() == sajson::TYPE_STRING)
		ret.order_status = order_status.as_cstring();

	sajson::value bet_id = node.get_value_of_key(sajson::literal("betId"));
	if (bet_id.get_type() == sajson::TYPE_STRING)
		ret.bet_id = bet_id.as_cstring();

	sajson::value placed_date = node.get_value_of_key(sajson::literal("placedDate"));
	if (placed_date.get_type() == sajson::TYPE_STRING)
		ret.placed_timestamp =
			parse_iso8601(placed_date.as_cstring(), placed_date.get_string_length());

	sajson::value av_price_matched =
		node.get_value_of_key(sajson::literal("averagePriceMatched"));
	if (av_price_matched.get_type() != sajson::TYPE_NULL)
		ret.average_price_matched = av_price_matched.get_number_value();

	sajson::value size_matched = node.get_value_of_key(sajson::literal("sizeMatched"));
	if (size_matched.get_type() != sajson::TYPE_NULL)
		ret.size_matched = size_matched.get_number_value();

	sajson::value instruction = node.get_value_of_key(sajson::literal("instruction"));
	if (instruction.get_type() == sajson::TYPE_OBJECT)
		ret.instruction = parse_place_instruction(instruction, response);
	else if (instruction.get_type() != sajson::TYPE_NULL)
		throw std::runtime_error(
			std::string(
				"placeOrders place instruction report has invalid instruction: ") +
			response);

	return ret;
}

static auto parse_place_execution_report(std::string& response) -> place_execution_result
{
	// Take a copy as sajson will mutate the string.
	std::string data = response;
	sajson::document doc = janus::internal::parse_json("", &data[0], data.size());
	const sajson::value& root = doc.get_root();
	if (root.get_type() != sajson::TYPE_OBJECT)
		throw std::runtime_error("placeOrders returned non-object response");

	place_execution_result ret{};

	std::string status = root.get_value_of_key(sajson::literal("status")).as_cstring();
	ret.status = status;

	if (status == "SUCCESS")
		ret.result = result_type::SUCCESS;
	else if (status == "FAILURE")
		ret.result = result_type::FAILURE;
	else
		ret.result = result_type::UNKNOWN;

	sajson::value err_code = root.get_value_of_key(sajson::literal("errorCode"));
	if (err_code.get_type() == sajson::TYPE_STRING)
		ret.error_code = err_code.as_cstring();

	sajson::value customer_ref = root.get_value_of_key(sajson::literal("customerRef"));
	if (customer_ref.get_type() == sajson::TYPE_STRING)
		ret.customer_ref = customer_ref.as_cstring();

	sajson::value reports = root.get_value_of_key(sajson::literal("instructionReports"));
	if (reports.get_type() != sajson::TYPE_ARRAY) {
		if (ret.result == result_type::SUCCESS)
			throw std::runtime_error(
				std::string(
					"placeOrders response has non-array instructions field: ") +
				response);

		// On error/unknown we would expect this to be missing or empty.
		return ret;
	}

	uint64_t num_reports = reports.get_length();
	if (num_reports == 0) {
		if (ret.result == result_type::SUCCESS)
			throw std::runtime_error(
				std::string("placeOrders response has empty instructions array: ") +
				response);

		// On error/unknown we would expect this to be missing or empty.
		return ret;
	} else if (num_reports > 1) {
		throw std::runtime_error(std::string("placeOrders response has ") +
					 std::to_string(num_reports) +
					 " instructions, expected 0: " + response);
	}

	sajson::value report = reports.get_array_element(0);
	if (report.get_type() != sajson::TYPE_OBJECT)
		throw std::runtime_error(
			std::string(
				"placeOrders instruction report has invalid instruction report:") +
			response);

	ret.report = parse_place_instruction_report(report, response);

	return ret;
}

auto place_order(session& session, uint64_t market_id, bet& bet, const std::string& strategy_ref,
		 const std::string& order_ref, bool fill_or_kill) -> place_execution_result
{
	std::string json =
		gen_place_order_json(market_id, bet, strategy_ref, order_ref, fill_or_kill);
	std::string response = session.api("placeOrders", json);
	return parse_place_execution_report(response);
}
} // namespace janus::betfair
