#pragma once

#include "error.hh"

#include <array>
#include <cstdint>
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

// Maximum size of mbedtls reported error.
static constexpr uint64_t MBEDTLS_ERROR_BUF_SIZE = 500;

// Generate error based on mbedtls error code.
static inline auto gen_err(const std::string& prefix, int err_code) -> network_error
{
	std::array<char, MBEDTLS_ERROR_BUF_SIZE> error_buf{};
	internal::mbedtls_strerror(err_code, &error_buf[0], MBEDTLS_ERROR_BUF_SIZE);

	return network_error(err_code, prefix, &error_buf[0]);
}
} // namespace janus::tls::internal
