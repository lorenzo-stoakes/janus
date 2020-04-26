#include "janus.hh"

#include <stdexcept>
#include <string>

namespace janus::tls
{
certs::certs() : _self_signed{false}, _loaded{false}
{
	internal::mbedtls_x509_crt_init(&_cacert);
	internal::mbedtls_pk_init(&_pk_context);
}

certs::~certs()
{
	internal::mbedtls_x509_crt_free(&_cacert);
	internal::mbedtls_pk_free(&_pk_context);
}

void certs::load(const char* cert_path)
{
	if (_loaded)
		throw std::runtime_error(std::string("Attempting to load ") + cert_path +
					 " but already loaded");

	load_cert(cert_path);
	_loaded = true;
}

void certs::load(const char* cert_path, const char* key_path)
{
	if (_loaded)
		throw std::runtime_error(std::string("Attempting to load ") + cert_path +
					 +" and key " + key_path + " but already loaded");

	load_cert(cert_path);
	_loaded = true;
	load_key(key_path);
	_self_signed = true;
}

void certs::self_sign(internal::mbedtls_ssl_config& conf)
{
	int err_num = internal::mbedtls_ssl_conf_own_cert(&conf, &_cacert, &_pk_context);
	if (err_num != 0)
		throw internal::gen_err("Self-signing certificate", err_num);
}

void certs::load_cert(const char* path)
{
	int err_num = internal::mbedtls_x509_crt_parse_file(&_cacert, path);
	if (err_num != 0)
		throw internal::gen_err(std::string("Parsing x.509 certificate at ") + path,
					err_num);
}

void certs::load_key(const char* path)
{
	int err_num = internal::mbedtls_pk_parse_keyfile(&_pk_context, path, nullptr);
	if (err_num != 0)
		throw internal::gen_err(std::string("Parsing key at ") + path, err_num);
}
} // namespace janus::tls