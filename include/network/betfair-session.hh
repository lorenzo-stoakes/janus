#pragma once

#include "config.hh"
#include "network/certs.hh"
#include "network/client.hh"
#include "network/http_request.hh"
#include "network/rng.hh"

#include <cstdint>
#include <memory>
#include <string>

namespace janus::betfair
{
// Login/logout buffer size.
static constexpr uint64_t DEFAULT_HTTP_BUF_SIZE = 1000;

// Represents a session with betfair.
// Note that as all the operations performed here are slow and network-limited
// anyway, we make no efforts to avoid allocations etc.
// Note that this is DEEPLY un-thread safe as a shared internal buffer is used.
class session
{
public:
	session(janus::tls::rng& rng, config& config)
		: _certs_loaded{false},
		  _logged_in{false},
		  _rng{rng},
		  _config{config},
		  _internal_buf{std::make_unique<char[]>(INTERNAL_BUFFER_SIZE)}
	{
	}

	// Load certificates for betfair operations.
	void load_certs();

	// Login using non-interactive betfair login endpoint.
	void login();

	// Logout, invalidating session token.
	void logout();

	// Query API endpoint. DEEPLY un-thread safe.
	auto api(const std::string& endpoint, const std::string& json) -> std::string;

	// Return a connection configured for the stream API, connected and
	// authenticated, ready for use.
	//   returns: Pair containing connection ID and connection.
	auto make_stream_connection(std::string& conn_id) -> janus::tls::client;

private:
	// Maximum number of characters for either username or password.
	static constexpr uint64_t MAX_USERNAME_PW_SIZE = 50;

	// Everything uses the standard TLS port.
	static constexpr const char* PORT = "443";

	// Maximum size of data we expect to receive in internal buffer.
	static constexpr int INTERNAL_BUFFER_SIZE = 10'000'000;

	// URLs and paths to API endpoints.

	static constexpr const char* ID_HOST = "identitysso-cert.betfair.com";
	static constexpr const char* LOGIN_PATH = "/api/certlogin";
	static constexpr const char* LOGOUT_PATH = "/api/logout";

	static constexpr const char* API_HOST = "api.betfair.com";
	static constexpr const char* API_PATH = "/exchange/betting/rest/v1.0";

	static constexpr const char* STREAM_HOST = "stream-api.betfair.com";

	bool _certs_loaded;
	bool _logged_in;
	janus::tls::rng& _rng;
	config& _config;
	janus::tls::certs _certs, _self_sign_cert;
	std::unique_ptr<char[]> _internal_buf;
	std::string _session_token;

	// Assert that the certificates are loaded, throw otherwise.
	void check_certs_loaded();

	// Assert that we're logged in, throw otherwise.
	void check_logged_in();

	// Generate an HTTP request for logging in.
	auto gen_login_req(char* buf, uint64_t cap) -> http_request;

	// Generate an HTTP request for logging out.
	auto gen_logout_req(char* buf, uint64_t cap) -> http_request;

	// Generate an HTTP request for an API-NG call.
	auto gen_api_req(const std::string& endpoint, const std::string& json) -> http_request;

	// Connect to and authenticate stream connection, ready for use.
	// Returns connection ID.
	auto authenticate_stream(janus::tls::client& client) -> std::string;

	// Read from connection into internal buffer until newline. Inclusive of
	// newline. Appends null terminator.
	//    client: Client connection to read from.
	//   returns: Numbers of bytes read.
	auto read_until_newline(janus::tls::client& client) -> int;
};
} // namespace janus::betfair
