#pragma once

#include "error.hh"

#include <string>

namespace janus::tls::internal
{
// Place includes in this namespace to reduce pollution of global namespace
// (macro pollution is unavoidable).
#include "mbedtls/certs.h"
#include "mbedtls/ctr_drbg.h"
#include "mbedtls/entropy.h"
#include "mbedtls/error.h"
#include "mbedtls/net_sockets.h"
#include "mbedtls/platform.h"
#include "mbedtls/ssl.h"

// Generate error based on mbedtls error code.
static inline auto gen_err(std::string prefix, int err_code) -> network_error
{
	char error_buf[500];
	internal::mbedtls_strerror(err_code, error_buf, 500);

	return network_error(err_code, prefix, error_buf);
}
} // namespace janus::tls::internal
