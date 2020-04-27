#pragma once

#include "config.hh"
#include "network/betfair-session.hh"
#include "network/client.hh"

#include <memory>
#include <string>
#include <vector>

namespace janus::betfair
{
class stream
{
public:
	explicit stream(session& session);

	// Get stream connection ID.
	auto connection_id() const -> std::string
	{
		return _conn_id;
	}

	// Subscribe to market updates using the provided filter JSON. Tolerate
	// allocation here since we only rarely need to invoke this.
	void market_subscribe(const std::string& filter_json, const std::string& data_filter_json);

	// Subscribe to up to all or the first MAX_MARKETS markets (whichever is
	// lesser) as specified using the specified data filter JSON. Tolerate
	// allocation here since we only rarely need to invoke this.
	void market_subscribe(const std::vector<std::string>& market_ids,
			      const std::string& data_filter_json);

	// Subscribe to markets according to config-specified filter and data
	// filter JSON. Returns full metadata for requested markets.
	auto market_subscribe(config& config) -> std::string;

	// Reads the next line of output from the internal buffer or stream,
	// placing a null terminator at the end of the line. This is BLOCKING
	// (intended to be run in a separate thread).
	auto read_next_line(int& size) -> char*;

	// Betfair only allow subscriptions up to 200 markets.
	static constexpr int MAX_MARKETS = 200;

private:
	// Maximum size of data we expect to receive in internal buffer.
	static constexpr int INTERNAL_BUFFER_SIZE = 10'000'000;

	int _read_offset;
	int _read_size;
	session& _session;
	std::string _conn_id;
	janus::tls::client _client;
	std::unique_ptr<char[]> _internal_buf;

	// Read the next line**S** of input from the stream into the internal
	// buffer (BLOCKING). This might read multiple lines but always contains
	// FULL lines. It sets _read_size to the length and _read_offset to 0.
	void read_next_lines();
};
} // namespace janus::betfair
