#include "janus.hh"

#include <gtest/gtest.h>
#include <stdexcept>

// We are limited in the tests we can perform as this would require a simulated
// betfair endpoint or rearchitecting to mock TLS. As bugs in relation to
// fundamental betfair network operations would be fairly obvious I am more
// relaxed about there being relatively less coverage here.

namespace
{
// Test that the betfair connection object behaves as expected.
TEST(betfair_test, connection)
{
	janus::tls::rng rng;

	janus::config config = janus::parse_config("../test/test-config/config3.json");

	janus::betfair::connection conn(rng, config);

	// Trying to load certificates with an unseeded RNG should fail.
	EXPECT_THROW(conn.load_certs(), std::runtime_error);
	rng.seed();
	// Once seeded everything should be ok.
	EXPECT_NO_THROW(conn.load_certs());

	// Further attempts to load certificates should be no-ops as we already
	// loaded them.
	EXPECT_NO_THROW(conn.load_certs());

	// Logging out when not logged in should be a no-op.
	EXPECT_NO_THROW(conn.logout());
}
} // namespace
