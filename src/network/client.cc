#include "janus.hh"

namespace janus::tls
{
client::client(const char* host, const char* port, certs& certs, rng& rng, uint32_t timeout_ms)
	: _host{host},
	  _port{port},
	  _timeout_ms{timeout_ms},
	  _certs{certs},
	  _rng{rng},
	  _connected{false},
	  _invalid{false},
	  _moved{false},
	  _server_fd{},
	  _ssl{},
	  _conf{}
{
	init();
}

client::~client()
{
	destroy();
}

client::client(client&& that) noexcept
	: _host{that._host},
	  _port{that._port},
	  _timeout_ms{that._timeout_ms},
	  _certs{that._certs},
	  _rng{that._rng},
	  _connected{that._connected},
	  _invalid{that._invalid},
	  _moved{false},
	  _server_fd{that._server_fd},
	  _ssl{that._ssl},
	  _conf{that._conf}
{
	that._moved = true;
}

void client::connect()
{
	check_valid();

	if (_connected)
		return;

	config();

	// Connect via TCP/IP.
	int err_num =
		internal::mbedtls_net_connect(&_server_fd, _host, _port, MBEDTLS_NET_PROTO_TCP);
	if (err_num != 0)
		throw gen_conn_err("Client", err_num);

	// Configure I/O.
	internal::mbedtls_ssl_set_bio(&_ssl, &_server_fd, internal::mbedtls_net_send,
				      internal::mbedtls_net_recv,
				      internal::mbedtls_net_recv_timeout);

	// TLS handshake.
	do {
		err_num = internal::mbedtls_ssl_handshake(&_ssl);
	} while (should_try_again(err_num));
	if (err_num != 0)
		throw gen_conn_err("Handshake", err_num);

	_connected = true;
}

void client::disconnect()
{
	if (!_connected || _invalid)
		return;

	_connected = false;
	_invalid = true;

	internal::mbedtls_ssl_close_notify(&_ssl);
}

auto client::read(char* buf, int size, bool& disconnected) -> int
{
	check_valid();
	check_connected("read");

	auto* ubuf = reinterpret_cast<unsigned char*>(buf);
	int err_num = internal::mbedtls_ssl_read(&_ssl, ubuf, size);

	if (should_try_again(err_num))
		return 0;

	if (err_num == MBEDTLS_ERR_SSL_PEER_CLOSE_NOTIFY || err_num == 0) {
		_invalid = true;
		_connected = false;
		disconnected = true;
		return 0;
	}

	if (err_num < 0)
		throw gen_conn_err("Read", err_num);

	disconnected = false;
	int bytes_read = err_num;
	return bytes_read;
}

void client::write(const char* buf, int size)
{
	check_valid();
	check_connected("write");

	int offset = 0;
	const auto* ubuf = reinterpret_cast<const unsigned char*>(buf);
	while (offset < size) {
		int err_num = internal::mbedtls_ssl_write(&_ssl, &ubuf[offset], size - offset);

		if (should_try_again(err_num))
			continue;

		if (err_num < 0)
			throw gen_conn_err("Write", err_num);

		int bytes_written = err_num;
		offset += bytes_written;
	}
}

void client::init()
{
	internal::mbedtls_net_init(&_server_fd);
	internal::mbedtls_ssl_init(&_ssl);
	internal::mbedtls_ssl_config_init(&_conf);
}

void client::destroy()
{
	if (_moved)
		return;

	// We disconnect on destroy.
	disconnect();

	internal::mbedtls_net_free(&_server_fd);
	internal::mbedtls_ssl_free(&_ssl);
	internal::mbedtls_ssl_config_free(&_conf);
}

void client::check_valid()
{
	if (_invalid)
		throw std::runtime_error("Attempt to use invalid client.");
}

void client::check_connected(const char* op)
{
	if (!_connected)
		throw std::runtime_error(std::string("Attempt to ") + op +
					 " on disconnected client.");
}

auto client::gen_conn_err(const std::string& prefix, int err_code) -> network_error
{
	// If we're generating a connection error then our underlying
	// mbedtls state is invalid.
	_invalid = true;

	std::string conn_prefix = std::string(_host) + ":" + _port + ": ";
	return internal::gen_err(conn_prefix + prefix, err_code);
}

auto client::should_try_again(int err_num) -> bool
{
	switch (err_num) {
	case MBEDTLS_ERR_SSL_WANT_READ:
	case MBEDTLS_ERR_SSL_WANT_WRITE:
	case MBEDTLS_ERR_SSL_ASYNC_IN_PROGRESS:
	case MBEDTLS_ERR_SSL_CRYPTO_IN_PROGRESS:
		return true;
	default:
		return false;
	}
}

void client::config()
{
	int err_num = internal::mbedtls_ssl_config_defaults(&_conf, MBEDTLS_SSL_IS_CLIENT,
							    MBEDTLS_SSL_TRANSPORT_STREAM,
							    MBEDTLS_SSL_PRESET_DEFAULT);
	if (err_num != 0)
		throw gen_conn_err("Initialising SSL config", err_num);

	// Set timeout.
	internal::mbedtls_ssl_conf_read_timeout(&_conf, _timeout_ms);

	// Set RNG.
	if (!_rng.seeded())
		throw std::runtime_error("RNG not seeded on connect");

	internal::mbedtls_ssl_conf_rng(&_conf, internal::mbedtls_ctr_drbg_random, &_rng.drbg());

	// Configure certificates.
	if (!_certs.loaded())
		throw std::runtime_error("Certificates not loaded on connect");
	if (_certs.self_signed()) {
		_certs.self_sign(_conf);
		// If we self-sign we don't need to verify the certificate.
		internal::mbedtls_ssl_conf_authmode(&_conf, MBEDTLS_SSL_VERIFY_NONE);
	} else {
		internal::mbedtls_ssl_conf_authmode(&_conf, MBEDTLS_SSL_VERIFY_REQUIRED);
	}
	internal::mbedtls_ssl_conf_ca_chain(&_conf, &_certs.chain(), nullptr);

	// Configure SSL object.
	err_num = internal::mbedtls_ssl_setup(&_ssl, &_conf);
	if (err_num != 0)
		throw gen_conn_err("SSL setup", err_num);
	err_num = internal::mbedtls_ssl_set_hostname(&_ssl, _host);
	if (err_num != 0)
		throw gen_conn_err("Set hostname", err_num);
}

auto client::read_until_newline(char* buf, int size) -> int
{
	bool disconnected = false;
	int offset = 0;
	do {
		offset += read(&buf[offset], size, disconnected);
	} while (!disconnected && offset < size && buf[offset - 1] != '\n');

	if (disconnected)
		throw std::runtime_error("Unexpected disconnection");

	// Leave an extra character for the null terminator.
	if (offset >= size)
		throw std::runtime_error("Read until newline exceeded buffer size?!");

	buf[offset] = '\0';
	return offset;
}

} // namespace janus::tls
