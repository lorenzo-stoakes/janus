#pragma once

#include <stdexcept>
#include <string>

#include "price_range.hh"

namespace janus
{
// Error in parsing JSON.
class json_parse_error : public std::exception
{
public:
	json_parse_error(const char* filename, int line, int col, const char* error)
		: _what{std::string("JSON parse error: ") + filename + ":" + std::to_string(line) +
			":" + std::to_string(col) + ": " + error}
	{
	}

	auto what() const noexcept -> const char* override
	{
		return _what.c_str();
	}

private:
	std::string _what;
};

class universe_apply_error : public std::exception
{
public:
	universe_apply_error(uint64_t market_id, uint64_t runner_id, std::exception& underlying)
		: _what{"m" + std::to_string(market_id) + ":" + "r" + std::to_string(runner_id) +
			": " + underlying.what()}
	{
	}

	auto what() const noexcept -> const char* override
	{
		return _what.c_str();
	}

private:
	std::string _what;
};

// Error in network operation.
class network_error : public std::exception
{
public:
	network_error(int err_code, const std::string& prefix, const char* err_msg)
		: _err_code{err_code}, _what{std::string("tls:") + prefix + ": " + err_msg}
	{
	}

	auto err_code() const -> int
	{
		return _err_code;
	}

	auto what() const noexcept -> const char* override
	{
		return _what.c_str();
	}

private:
	int _err_code;
	std::string _what;
};

namespace betfair
{
// Error in trying to add unmatched volume to a ladder.
class invalid_unmatched_update : public std::exception
{
public:
	invalid_unmatched_update(uint64_t index, double vol, uint64_t limit_index, double limit_vol)
		: _what{"Invalid unmatched update, "}
	{
		betfair::price_range range;
		double price = betfair::price_range::index_to_price(index);

		if (vol < 0) { // ATB
			double min_atl = betfair::price_range::index_to_price(limit_index);

			_what += "ATB update at price " + std::to_string(price);
			_what += " of volume " + std::to_string(vol);
			_what += " is greater than minimum ATL of " + std::to_string(min_atl);
			_what += " of volume " + std::to_string(limit_vol);
		} else if (vol > 0) { // ATL
			double max_atb = betfair::price_range::index_to_price(limit_index);

			_what += "ATL update at price " + std::to_string(price);
			_what += " of volume " + std::to_string(vol);
			_what += " is less than maximum ATB of " + std::to_string(max_atb);
			_what += " of volume " + std::to_string(limit_vol);
		} else { // Clear?? Shouldn't occur.
			_what += "Unexpected clear at " + std::to_string(price);
		}
	}

	auto what() const noexcept -> const char* override
	{
		return _what.c_str();
	}

private:
	std::string _what;
};
} // namespace betfair
} // namespace janus
