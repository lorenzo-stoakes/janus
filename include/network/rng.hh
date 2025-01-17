#pragma once

#include "network/internal.hh"

namespace janus::tls
{
// Cryptographic RNG for use in TLS connection.
class rng
{
public:
	rng();
	~rng();
	rng(rng&& that) noexcept;
	auto operator=(rng&& that) noexcept -> rng&;

	// No copying.
	rng(const rng& that) = delete;
	auto operator=(const rng& that) -> rng& = delete;

	// Get entropy context. Invalid if not seeded.
	auto entropy() -> internal::mbedtls_entropy_context&
	{
		return _entropy;
	}

	// Get Deterministic Random Bit Generator (DRBG). Invalid if not seeded.
	auto drbg() -> internal::mbedtls_ctr_drbg_context&
	{
		return _ctr_drbg;
	}

	// Is this RNG seeded?
	auto seeded() const -> bool
	{
		return _seeded;
	}

	// Seed RNG.
	void seed();

private:
	bool _seeded;
	bool _moved;

	internal::mbedtls_entropy_context _entropy;
	internal::mbedtls_ctr_drbg_context _ctr_drbg;

	// Destroy instance, freeing all allocated memory.
	void destroy();
};
} // namespace janus::tls
