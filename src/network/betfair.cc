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

static auto get_response(const char* action, janus::tls::client& client, char* buf, int size,
			 uint64_t& data_size) -> char*
{
	bool disconnected = false;
	int bytes = client.read(buf, size, disconnected);
	if (bytes == 0)
		throw std::runtime_error("Received no reply on login");

	int response_code = 0;
	uint64_t offset = 0;
	int bytes_remaining = parse_http_response(buf, bytes, response_code, offset);

	check_response_code(action, response_code, buf, bytes, offset);

	int additional_space = size - bytes;
	if (bytes_remaining > additional_space)
		throw std::runtime_error("Unexpected further " + std::to_string(bytes_remaining) +
					 " bytes for " + action + ":\n" + buf);

	int write_offset = bytes;
	while (bytes_remaining > 0) {
		int further_bytes =
			client.read(&buf[write_offset], size - write_offset, disconnected);
		write_offset += further_bytes;
		bytes_remaining -= further_bytes;
	}

	// Terminate data.
	if (offset > 0) {
		if (write_offset == size)
			buf[write_offset - 1] = '\0';
		else
			buf[write_offset] = '\0';
	}

	data_size = write_offset - offset;
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
	char* ptr = get_response("login", client, buf, DEFAULT_HTTP_BUF_SIZE, size);

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
	char* ptr = get_response("logout", client, buf, DEFAULT_HTTP_BUF_SIZE, size);
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

auto session::api(const std::string& endpoint, const std::string& json) -> std::string
{
	check_logged_in();

	http_request req = gen_api_req(endpoint, json);

	janus::tls::client client(API_HOST, PORT, _certs, _rng);
	client.connect();
	client.write(req.buf(), req.size());

	uint64_t size;
	char* ptr = get_response("API", client, _internal_buf.get(), INTERNAL_BUFFER_SIZE, size);
	return std::string(ptr, size);
}

auto session::authenticate_stream(janus::tls::client& client) -> std::string
{
	check_logged_in();
	if (client.connected())
		throw std::runtime_error("Cannot authenticate stream, already connected!");

	client.connect();

	// We receive a welcome message containing the connection ID.

	int bytes = read_until_newline(client);

	sajson::document doc = janus::internal::parse_json("", _internal_buf.get(), bytes);
	const sajson::value& root = doc.get_root();

	sajson::value op = root.get_value_of_key(sajson::literal("op"));
	if (op.get_type() != sajson::TYPE_STRING)
		throw std::runtime_error("Missing op in stream response");

	if (::strcmp(op.as_cstring(), "connection") != 0)
		throw std::runtime_error(std::string("Unexpected op (expected connection) ") +
					 op.as_cstring());

	sajson::value id = root.get_value_of_key(sajson::literal("connectionId"));
	if (id.get_type() != sajson::TYPE_STRING)
		throw std::runtime_error("Missing connectionId in stream response");

	std::string ret = id.as_string();

	// Now we need to authenticate.

	// TODO(lorenzo): While we aren't so worried about allocations at this
	// non-hotspot, this feels particularly horrible.
	std::string msg = R"({"op":"authentication","id":1,"appKey":")";
	msg += _config.app_key;
	msg += R"(","session":")";
	msg += _session_token;
	msg += "\"}\n";

	client.write(msg.c_str(), msg.size());

	// Receive response to authentication.
	bytes = read_until_newline(client);

	sajson::document auth_doc = janus::internal::parse_json("", _internal_buf.get(), bytes);
	const sajson::value& auth_root = auth_doc.get_root();

	sajson::value auth_op = auth_root.get_value_of_key(sajson::literal("op"));
	if (auth_op.get_type() != sajson::TYPE_STRING)
		throw std::runtime_error("Missing op in authentication stream response");

	if (::strcmp(auth_op.as_cstring(), "status") != 0)
		throw std::runtime_error(std::string("Unexpected op (expected status) ") +
					 auth_op.as_cstring());

	sajson::value status_code = auth_root.get_value_of_key(sajson::literal("statusCode"));
	if (::strcmp(status_code.as_cstring(), "SUCCESS") != 0)
		throw std::runtime_error(std::string("Unsuccessful authentication: ") +
					 status_code.as_cstring());

	return ret;
}

auto session::read_until_newline(janus::tls::client& client) -> int
{
	bool disconnected;
	int offset = 0;
	do {
		offset += client.read(&_internal_buf[offset], INTERNAL_BUFFER_SIZE, disconnected);
	} while (!disconnected && _internal_buf[offset - 1] != '\n');
	_internal_buf[offset] = '\0';

	if (disconnected)
		throw std::runtime_error("Unexpected disconnection");

	return offset;
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

auto session::gen_api_req(const std::string& endpoint, const std::string& json) -> http_request
{
	http_request req(_internal_buf.get(), INTERNAL_BUFFER_SIZE);
	// API calls have a trailing /.
	std::string path = std::string(API_PATH) + "/" + endpoint + "/";
	req.post(API_HOST, path.c_str());
	req.add_header("Accept", "application/json");
	req.add_header("X-Application", _config.app_key.c_str());
	req.add_header("X-Authentication", _session_token.c_str());
	req.add_header("Content-Type", "application/json");
	req.add_data(json.c_str(), json.size());

	return req;
}

void session::check_logged_in()
{
	if (!_logged_in)
		throw std::runtime_error("Attempting operation when not logged in");
}
} // namespace janus::betfair
