#pragma once

#include <stdexcept>
#include <string>

namespace janus
{
// Error in parsing JSON.
class json_parse_error : public std::exception
{
public:
	json_parse_error(const char* filename, int row, int col, const char* error)
		: _what{std::string("JSON parse error: ") + filename + ":" + std::to_string(row) +
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
} // namespace janus
