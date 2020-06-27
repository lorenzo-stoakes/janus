#pragma once

#include <cstdint>
#include <iostream>
#include <string>
#include <utility>
#include <vector>

#include "bet.hh"
#include "network/betfair-session.hh"

namespace janus::betfair
{
// The maximum number of markets we can request at once. Betfair-imposed data limit.
static constexpr uint64_t MAX_METADATA_REQUESTS = 100;

enum class result_type : uint64_t
{
	SUCCESS = 0,
	FAILURE = 1,
	UNKNOWN = 2
};

struct limit_order
{
	double size;
	double price;
	bet_persist_type persist;
	bool fill_or_kill;
};

struct place_instruction
{
	std::string order_type;
	uint64_t selection_id;
	bool is_back;
	std::string customer_order_ref;
	limit_order order;
};

struct place_instruction_report
{
	result_type result;
	std::string status;
	std::string error_code;
	std::string order_status;
	place_instruction instruction;
	std::string bet_id;
	uint64_t placed_timestamp;
	double average_price_matched;
	double size_matched;
};

struct place_execution_result
{
	result_type result;
	std::string status;
	std::string error_code;
	std::string customer_ref;
	place_instruction_report report;
};

// Helper function to assist debug - dump place execution report contents.
static inline void dump_place_execution_result(place_execution_result& res)
{
	std::cout << "result=" << static_cast<uint64_t>(res.result) << std::endl;
	std::cout << "status=" << res.status << std::endl;
	std::cout << "error_code=" << res.error_code << std::endl;
	std::cout << "customer_ref=" << res.customer_ref << std::endl;

	std::cout << "report.result=" << static_cast<uint64_t>(res.report.result) << std::endl;
	std::cout << "report.status=" << res.report.status << std::endl;
	std::cout << "report.error_code=" << res.report.error_code << std::endl;
	std::cout << "report.order_status=" << res.report.order_status << std::endl;
	std::cout << "report.bet_id=" << res.report.bet_id << std::endl;
	std::cout << "report.placed_timestamp=" << res.report.placed_timestamp << std::endl;
	std::cout << "report.average_price_matched=" << res.report.average_price_matched
		  << std::endl;
	std::cout << "report.size_matched=" << res.report.size_matched << std::endl;

	std::cout << "report.instruction.order_type=" << res.report.instruction.order_type
		  << std::endl;
	std::cout << "report.instruction.selection_id=" << res.report.instruction.selection_id
		  << std::endl;
	if (res.report.instruction.is_back)
		std::cout << "report.instruction.is_back=BACK" << std::endl;
	else
		std::cout << "report.instruction.is_back=LAY" << std::endl;
	std::cout << "report.instruction.customer_order_ref="
		  << res.report.instruction.customer_order_ref << std::endl;

	std::cout << "report.instruction.order.size=" << res.report.instruction.order.size
		  << std::endl;
	std::cout << "report.instruction.order.price=" << res.report.instruction.order.price
		  << std::endl;
	if (res.report.instruction.order.persist == bet_persist_type::LAPSE)
		std::cout << "report.instruction.order.persist=LAPSE" << std::endl;
	else if (res.report.instruction.order.persist == bet_persist_type::PERSIST)
		std::cout << "report.instruction.order.persist=PERSIST" << std::endl;
	else if (res.report.instruction.order.persist == bet_persist_type::MARKET_ON_CLOSE)
		std::cout << "report.instruction.order.persist=MARKET_ON_CLOSE" << std::endl;

	if (res.report.instruction.order.fill_or_kill)
		std::cout << "report.instruction.order.fill_or_kill=true" << std::endl;
	else
		std::cout << "report.instruction.order.fill_or_kill=false" << std::endl;
}

// Retrieve market IDs using the specified market filter JSON, returned in date
// order. If the results exceed 1000, then only the first 1000 are returned.
// https://docs.developer.betfair.com/display/1smk3cen4v3lu3yomq5qye0ni/Betting+Type+Definitions#BettingTypeDefinitions-MarketFilter
auto get_market_ids(session& session, const std::string& filter_json)
	-> std::pair<std::vector<std::string>, std::string>;

// Place an order with the specified bet in the market.
auto place_order(session& session, uint64_t market_id, bet& bet,
		 const std::string& strategy_ref = "", const std::string& order_ref = "",
		 bool fill_or_kill = false) -> place_execution_result;
} // namespace janus::betfair
