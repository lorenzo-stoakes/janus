#pragma once

#include "config.hh"
#include "network/certs.hh"
#include "network/client.hh"
#include "network/http_request.hh"
#include "network/rng.hh"

namespace janus::betfair
{
// Login buffer size.
static constexpr uint64_t DEFAULT_HTTP_BUF_SIZE = 1000;

class session
{
public:
	session(janus::tls::rng& rng, config& config)
		: _certs_loaded{false}, _logged_in{false}, _rng{rng}, _config{config}
	{
	}

	// Load certificates for betfair operations.
	void load_certs();

	// Login using non-interactive betfair login endpoint.
	void login();

	// Logout, invalidating session token.
	void logout();

	// Get current session token if logged in.
	auto session_token() -> std::string
	{
		return _session_token;
	}

	static constexpr const char* ID_HOST = "identitysso-cert.betfair.com";
	static constexpr const char* LOGIN_PATH = "/api/certlogin";
	static constexpr const char* LOGOUT_PATH = "/api/logout";

	static constexpr const char* API_HOST = "api.betfair.com";
	static constexpr const char* API_PATH = "/exchange/betting/rest/v1.0";

	static constexpr const char* STREAM_HOST = "stream-api.betfair.com";

private:
	// Maximum number of characters for either username or password.
	static constexpr uint64_t MAX_USERNAME_PW_SIZE = 50;

	// Everything uses the standard TLS port.
	static constexpr const char* PORT = "443";

	bool _certs_loaded;
	bool _logged_in;
	janus::tls::rng& _rng;
	config& _config;
	janus::tls::certs _certs, _self_sign_cert;

	// We tolerate an allocate as logging in/out is slow anyway.
	std::string _session_token;

	// Assert that the certificates are loaded, throw otherwise.
	void check_certs_loaded();

	// Generate an HTTP request for logging in.
	auto gen_login_req(char* buf, uint64_t cap) -> http_request;

	// Generate an HTTP request for logging out.
	auto gen_logout_req(char* buf, uint64_t cap) -> http_request;
};
} // namespace janus::betfair
