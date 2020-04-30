#include "janus.hh"

namespace janus::tls
{
rng::rng() : _seeded(false), _moved{false}, _entropy{}, _ctr_drbg{}
{
	internal::mbedtls_entropy_init(&_entropy);
	internal::mbedtls_ctr_drbg_init(&_ctr_drbg);
}

rng::rng(rng&& that) noexcept
	: _seeded{that._seeded}, _moved{false}, _entropy{that._entropy}, _ctr_drbg{that._ctr_drbg}
{
	that._moved = true;
}

auto rng::operator=(rng&& that) noexcept -> rng&
{
	if (this == &that)
		return *this;

	destroy();

	_seeded = that._seeded;
	_entropy = that._entropy;
	_ctr_drbg = that._ctr_drbg;

	that._moved = true;

	return *this;
}

rng::~rng()
{
	destroy();
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

void rng::destroy()
{
	if (_moved)
		return;

	internal::mbedtls_entropy_free(&_entropy);
	internal::mbedtls_ctr_drbg_free(&_ctr_drbg);
}
} // namespace janus::tls
