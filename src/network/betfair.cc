#include "janus.hh"

#include "sajson.hh"
#include <cstdint>
#include <cstring>
#include <stdexcept>
#include <string>

namespace janus::betfair
{
void session::load_certs()
{
	if (_certs_loaded)
		return;

	if (!_rng.seeded())
		throw std::runtime_error("RNG not seeded");

	// We use _certs for standard operations and _self_sign_cert for
	// unattended login.
	_certs.load();
	_self_sign_cert.load(_config.cert_path.c_str(), _config.key_path.c_str());
	_certs_loaded = true;
}

static void check_response_code(const char* action, int response_code, char* buf, int bytes,
				uint64_t offset)
{
	if (response_code == 200)
		return;

	std::string err =
		"Received " + std::to_string(response_code) + " response code on " + action;
	if (offset > 0)
		err += std::string(":\n") + buf;

	throw std::runtime_error(err);
}

static auto get_response(const char* action, janus::tls::client& client, char* buf,
			 uint64_t& data_size) -> char*
{
	bool disconnected = false;
	// Reuse buffer as already sent write data.
	int bytes = client.read(buf, DEFAULT_HTTP_BUF_SIZE, disconnected);
	if (bytes == 0)
		throw std::runtime_error("Received no reply on login");

	int response_code = 0;
	uint64_t offset = 0;
	uint64_t bytes_remaining = parse_http_response(buf, bytes, response_code, offset);

	check_response_code(action, response_code, buf, bytes, offset);

	// The response should NEVER exceed our buffer size.
	if (bytes_remaining > 0)
		throw std::runtime_error("Unexpected further " + std::to_string(bytes_remaining) +
					 " bytes for " + action + ":\n" + buf);

	// Terminate data.
	if (offset > 0) {
		if (bytes == DEFAULT_HTTP_BUF_SIZE)
			buf[bytes - 1] = '\0';
		else
			buf[bytes] = '\0';
	}

	data_size = bytes - offset;
	return &buf[offset];
}

void session::login()
{
	// https://docs.developer.betfair.com/display/1smk3cen4v3lu3yomq5qye0ni/Non-Interactive+%28bot%29+login

	if (_logged_in)
		return;
	check_certs_loaded();

	char buf[DEFAULT_HTTP_BUF_SIZE];
	http_request req = gen_login_req(buf, DEFAULT_HTTP_BUF_SIZE);

	janus::tls::client client(ID_HOST, PORT, _self_sign_cert, _rng);
	client.connect();
	client.write(req.buf(), req.size());

	uint64_t size;
	char* ptr = get_response("login", client, buf, size);

	sajson::document doc = janus::internal::parse_json("", ptr, size);
	const sajson::value& root = doc.get_root();

	sajson::value login_status = root.get_value_of_key(sajson::literal("loginStatus"));
	if (login_status.get_type() != sajson::TYPE_STRING)
		throw std::runtime_error(std::string("Cannot find login status in ") + buf);

	if (::strcmp(login_status.as_cstring(), "SUCCESS") != 0)
		throw std::runtime_error(std::string("Unsuccessful login: ") + buf);

	sajson::value session_token = root.get_value_of_key(sajson::literal("sessionToken"));
	if (session_token.get_type() != sajson::TYPE_STRING)
		throw std::runtime_error(std::string("Cannot find session token in ") + buf);

	_session_token = session_token.as_cstring();
	_logged_in = true;
}

void session::logout()
{
	if (!_logged_in)
		return;

	char buf[DEFAULT_HTTP_BUF_SIZE];
	http_request req = gen_logout_req(buf, DEFAULT_HTTP_BUF_SIZE);

	janus::tls::client client(ID_HOST, PORT, _certs, _rng);
	client.connect();
	client.write(req.buf(), req.size());

	uint64_t size;
	char* ptr = get_response("logout", client, buf, size);
	sajson::document doc = janus::internal::parse_json("", ptr, size);
	const sajson::value& root = doc.get_root();

	sajson::value status = root.get_value_of_key(sajson::literal("status"));
	if (status.get_type() != sajson::TYPE_STRING)
		throw std::runtime_error(std::string("Cannot find logout status in ") + buf);

	if (::strcmp(status.as_cstring(), "SUCCESS") != 0)
		throw std::runtime_error(std::string("Unsuccessful logout: ") + buf);

	_session_token = "";
	_logged_in = false;
}

void session::check_certs_loaded()
{
	if (!_certs_loaded)
		throw std::runtime_error("Attempting operation without certificates loaded");
}

auto session::gen_login_req(char* buf, uint64_t cap) -> http_request
{
	if (_config.username.size() > MAX_USERNAME_PW_SIZE ||
	    _config.password.size() > MAX_USERNAME_PW_SIZE)
		throw std::runtime_error("Username/password too long");

	http_request req(buf, cap);
	req.post(ID_HOST, LOGIN_PATH);
	req.add_header("Accept", "application/json");
	req.add_header("Content-Type", "application/x-www-form-urlencoded");
	req.add_header("X-Application", _config.app_key.c_str());

	// Leave room for form-encoded username/password specifier.
	char content_buf[2 * MAX_USERNAME_PW_SIZE + 20 + 2 + 1];

	const char username_prefix[] = "username=";
	uint64_t username_prefix_size = sizeof(username_prefix) - 1;
	::memcpy(content_buf, username_prefix, username_prefix_size);
	uint64_t offset = username_prefix_size;

	std::string& username = _config.username;
	::memcpy(&content_buf[offset], username.c_str(), username.size());
	offset += username.size();

	const char password_prefix[] = "&password=";
	uint64_t password_prefix_size = sizeof(password_prefix) - 1;
	::memcpy(&content_buf[offset], password_prefix, password_prefix_size);
	offset += password_prefix_size;

	std::string& password = _config.password;
	::memcpy(&content_buf[offset], password.c_str(), password.size());

	req.add_data(content_buf, offset + password.size());

	return req;
}

auto session::gen_logout_req(char* buf, uint64_t cap) -> http_request
{
	http_request req(buf, cap);
	req.post(ID_HOST, LOGOUT_PATH);
	req.add_header("Accept", "application/json");
	req.add_header("X-Application", _config.app_key.c_str());
	req.add_header("X-Authentication", _session_token.c_str());
	req.terminate();

	return req;
}
} // namespace janus::betfair
