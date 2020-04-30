#pragma once

#include "network/internal.hh"

namespace janus::tls
{
// Representation of certificate(s) for use in TLS connections.
class certs
{
public:
	certs();
	~certs();
	certs(certs&& that) noexcept;
	auto operator=(certs&& that) noexcept -> certs&;

	// No copying.
	certs(const certs& that) = delete;
	auto operator=(const certs& that) -> certs& = delete;

	// Is this a self-signed certificate/key pair?
	auto self_signed() const -> bool
	{
		return _self_signed;
	}

	// Retrieve certificate chain.
	auto chain() -> internal::mbedtls_x509_crt&
	{
		return _cacert;
	}

	// Retrieve private key (only valid if self-signed).
	auto key() -> internal::mbedtls_pk_context&
	{
		return _pk_context;
	}

	// Have we loaded certificate(s) yet?
	auto loaded() const -> bool
	{
		return _loaded;
	}

	// Load ROOT certificates.
	void load(const char* cert_path = ROOT_CA_PATH);

	// Load SELF-SIGNED certificate(s) and private key.
	void load(const char* cert_path, const char* key_path);

	// Self-sign certificate for specific SSL configuration.
	void self_sign(internal::mbedtls_ssl_config& conf);

	static constexpr const char* ROOT_CA_PATH =
		"/etc/ca-certificates/extracted/tls-ca-bundle.pem";

private:
	bool _self_signed;
	bool _loaded;
	bool _moved;
	internal::mbedtls_x509_crt _cacert;
	internal::mbedtls_pk_context _pk_context;

	// Load certificate(s) from specific path, throws on error.
	void load_cert(const char* path);

	// Load private key from specific path, throws on error.
	void load_key(const char* path);

	// Destroy instance, freeing all allocated memory.
	void destroy();
};
} // namespace janus::tls
