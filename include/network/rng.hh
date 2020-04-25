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

	// No copying, moving, assigning.
	rng(const rng& that) = delete;
	rng(rng&& that) = delete;
	auto operator=(const rng& that) -> rng& = delete;
	auto operator=(rng&& that) -> rng& = delete;

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
	const char* _name;
	int _name_size;
	bool _seeded;

	internal::mbedtls_entropy_context _entropy;
	internal::mbedtls_ctr_drbg_context _ctr_drbg;
};
} // namespace janus::tls
