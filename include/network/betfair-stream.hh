#pragma once

#include "network/betfair-session.hh"
#include "network/client.hh"

namespace janus::betfair
{
class stream
{
public:
	stream(session& session);

	// Get stream connection ID.
	auto connection_id() const -> std::string
	{
		return _conn_id;
	}

private:
	session& _session;
	std::string _conn_id;
	janus::tls::client _client;
};
} // namespace janus::betfair
