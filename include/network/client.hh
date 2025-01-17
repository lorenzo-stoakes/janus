#pragma once

#include "network/internal.hh"

#include "network/certs.hh"
#include "network/rng.hh"

namespace janus::tls
{
class client
{
public:
	client(const char* host, const char* port, certs& certs, rng& rng,
	       uint32_t timeout_ms = DEFAULT_TIMEOUT_MS);
	~client();
	// We can move ctor since moving another connection to our own makes
	// sense while ours is uninitialised.
	client(client&& that) noexcept;

	// No copying. It makes no sense to throw away this connection and copy
	// another over it.
	client(const client& that) = delete;
	auto operator=(const client& that) -> client& = delete;
	// No move-assign, it makes no sense to overwrite an existing connection
	// with another.
	auto operator=(client&& that) -> client& = delete;

	// Is this client valid (i.e. usable)?
	auto valid() const -> bool
	{
		return !_invalid;
	}

	// Is the client connected?
	auto connected() const -> bool
	{
		return _connected;
	}

	// Configured timeout in ms.
	auto timeout_ms() const -> uint32_t
	{
		return _timeout_ms;
	}

	// Connect to host and perform TLS handshake. BLOCKING.
	void connect();

	// Disconnect and invalidate the connection. BLOCKING.
	void disconnect();

	// Perform a BLOCKING read from the TLS connection. Returns as many
	// bytes as it receives (possibly 0), placing the received data in the
	// specified bufer.
	// If the connection is closed, the disconnected parameter is set and
	// the connection is invalidated.
	auto read(char* buf, int size, bool& disconnected) -> int;

	// Performs a BLOCKING read from the TLS connection and reads into the
	// specified buffer until a newline is found AT THE END OF THE DATA
	// SENT. Note this won't search for a newline _within_ a block of data,
	// rather it is for cases where the server signifies end of transmission
	// using a newline. If the connection is closed or the buffer is
	// exceeded, the function throws.
	// Returns the number of bytes read.
	auto read_until_newline(char* buf, int size) -> int;

	// Perform a BLOCKING write to the TLS connection. It will keep on
	// trying to write until write is complete or an error occurs.
	void write(const char* buf, int size);

	static constexpr uint32_t DEFAULT_TIMEOUT_MS = 5000;

private:
	const char* _host;
	const char* _port;
	uint32_t _timeout_ms;
	certs& _certs;
	rng& _rng;
	bool _connected;
	bool _invalid;
	bool _moved;

	internal::mbedtls_net_context _server_fd;
	internal::mbedtls_ssl_context _ssl;
	internal::mbedtls_ssl_config _conf;

	// Initialise mbedtls state.
	void init();

	// Disconnect then destroy underlying mbedtls state.
	void destroy();

	// Check whether the client object is valid.
	void check_valid();

	// Check whether the client object is connected for speciifed operation.
	void check_connected(const char* op);

	// Generate network error with prefix indicating host and port and sets
	// this client instance to invalid.
	auto gen_conn_err(const std::string& prefix, int err_code) -> network_error;

	// Does this error code indicate we should try the operation again?
	static auto should_try_again(int err_num) -> bool;

	void config();
};
} // namespace janus::tls
