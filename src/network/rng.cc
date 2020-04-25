#include "janus.hh"

namespace janus::tls
{
rng::rng() : _seeded(false)
{
	internal::mbedtls_entropy_init(&_entropy);
	internal::mbedtls_ctr_drbg_init(&_ctr_drbg);
}

rng::~rng()
{
	internal::mbedtls_entropy_free(&_entropy);
	internal::mbedtls_ctr_drbg_free(&_ctr_drbg);
}

void rng::seed()
{
	if (_seeded)
		return;

	int err_num = internal::mbedtls_ctr_drbg_seed(&_ctr_drbg, internal::mbedtls_entropy_func,
						      &_entropy, nullptr, 0);
	if (err_num != 0)
		throw internal::gen_err("Seeding RNG", err_num);

	_seeded = true;
}
} // namespace janus::tls
