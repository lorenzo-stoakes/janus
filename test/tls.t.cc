#include "janus.hh"

#include <cstdint>
#include <gtest/gtest.h>
#include <stdexcept>

#include <iostream>

// We perform limited tests on the TLS code, as testing it more thoroughly would
// require a TLS server and more of an end-to-end test setup.

// Much of the fundamentals of this are provided by the mbedtls library which we
// assume is working correctly.

namespace
{
// Test that the RNG
TEST(tls_test, rng)
{
	janus::tls::rng rng;

	EXPECT_FALSE(rng.seeded());
	ASSERT_NO_THROW(rng.seed());
	EXPECT_TRUE(rng.seeded());

	// Trying to seed again will just be a no-op.
	ASSERT_NO_THROW(rng.seed());
}

// Does the 'certs' TLS certificate chain object work correctly?
TEST(tls_test, certs)
{
	janus::tls::certs certs1;
	EXPECT_FALSE(certs1.loaded());
	EXPECT_FALSE(certs1.self_signed());

	// TODO(lorenzo): Can't assume we are in ./build directory.
	ASSERT_NO_THROW(certs1.load("../test/test-cert/client-2048.crt"));
	EXPECT_TRUE(certs1.loaded());
	EXPECT_FALSE(certs1.self_signed());

	// Trying to load again should fail.
	EXPECT_THROW(certs1.load("../test/test-cert/client-2048.crt"), std::runtime_error);
	// Trying to load again self-signed should fail too.
	EXPECT_THROW(certs1.load("../test/test-cert/client-2048.crt",
				 "../test/test-cert/client-2048.key"),
		     std::runtime_error);

	janus::tls::certs certs2;
	ASSERT_NO_THROW(certs2.load("../test/test-cert/client-2048.crt",
				    "../test/test-cert/client-2048.key"));
	EXPECT_TRUE(certs2.loaded());
	EXPECT_TRUE(certs2.self_signed());

	// Trying to load again should fail.
	EXPECT_THROW(certs2.load("../test/test-cert/client-2048.crt"), std::runtime_error);
	// Trying to load again self-signed should fail too.
	EXPECT_THROW(certs2.load("../test/test-cert/client-2048.crt",
				 "../test/test-cert/client-2048.key"),
		     std::runtime_error);

	janus::tls::certs certs3;
	ASSERT_NO_THROW(certs3.load()); // Loads default root certificates.
	EXPECT_TRUE(certs3.loaded());
	EXPECT_FALSE(certs3.self_signed());
}

// Does the client TLS connection object work correctly? This connects to an
// external website (mine :), which isn't great but not the end of the world.
TEST(tls_test, client)
{
	janus::tls::rng rng;
	janus::tls::certs certs;

	// We haven't seeded the RNG or loaded the certificates attempting to
	// connect should throw.
	janus::tls::client client1("ljs.io", "443", certs, rng);
	EXPECT_TRUE(client1.valid());
	EXPECT_FALSE(client1.connected());
	ASSERT_THROW(client1.connect(), std::runtime_error);
	// We should still be valid because this can be fixed.
	EXPECT_TRUE(client1.valid());
	EXPECT_FALSE(client1.connected());
	rng.seed();
	// We are now seeded but the certificates are still not loaded so we should still fail.
	ASSERT_THROW(client1.connect(), std::runtime_error);
	// We should still be valid because this can be fixed.
	EXPECT_TRUE(client1.valid());
	EXPECT_FALSE(client1.connected());

	// Now certificates are loaded we should be good to go.
	certs.load(); // Will load default root certificates.
	ASSERT_NO_THROW(client1.connect());
	EXPECT_TRUE(client1.valid());
	EXPECT_TRUE(client1.connected());

	// Disconnect should result in the client becoming invalid and
	// disconnected.
	ASSERT_NO_THROW(client1.disconnect());
	EXPECT_FALSE(client1.valid());
	EXPECT_FALSE(client1.connected());

	// Disconnected again should be a no-op.
	ASSERT_NO_THROW(client1.disconnect());

	char buf[500];
	bool disconnected = false;
	// As the object is invalid everything else should throw.
	EXPECT_THROW(client1.connect(), std::runtime_error);
	EXPECT_THROW(client1.read(buf, sizeof(buf), disconnected), std::runtime_error);
	EXPECT_THROW(client1.write("test", 4), std::runtime_error);

	janus::tls::client client2("ljs.io", "443", certs, rng);
	EXPECT_TRUE(client2.valid());
	EXPECT_FALSE(client2.connected());
	EXPECT_EQ(client2.timeout_ms(), janus::tls::client::DEFAULT_TIMEOUT_MS);

	ASSERT_NO_THROW(client2.connect());
	EXPECT_TRUE(client2.valid());
	EXPECT_TRUE(client2.connected());
	// A second attempt at connecting should be a no-op.
	ASSERT_NO_THROW(client2.connect());
	EXPECT_TRUE(client2.valid());
	EXPECT_TRUE(client2.connected());

	// Now actually try and retrieve something.
	char write_buf[] = "GET /test.txt HTTP/1.0\r\n\r\n";
	ASSERT_NO_THROW(client2.write(write_buf, sizeof(write_buf) - 1));

	disconnected = true;
	int count;
	ASSERT_NO_THROW(count = client2.read(buf, sizeof(buf), disconnected));
	ASSERT_EQ(disconnected, false);
	ASSERT_EQ(count, 500);

	// Skip headers.
	std::string str(buf, sizeof(buf));
	auto data_start = str.find("\r\n\r\n");
	EXPECT_NE(data_start, std::string::npos);
	// Skip newlines.
	data_start += 4;

	char chr = '0';
	// The test file contains the numbers '0123456789' repeated 100 times.
	for (uint64_t i = 0; i < sizeof(buf) - data_start; i++) {
		chr = '0' + (i % 10);
		ASSERT_EQ(buf[data_start + i], chr);
	}

	// Read next block of data.
	ASSERT_NO_THROW(count = client2.read(buf, sizeof(buf), disconnected));
	ASSERT_EQ(disconnected, false);
	// Will be at least 500 as we have 1000 bytes of data + headers.
	ASSERT_EQ(count, 500);

	for (uint64_t i = 0; i < sizeof(buf); i++) {
		if (chr == '9')
			chr = '0';
		else
			chr++;

		ASSERT_EQ(buf[i], chr);
	}

	// Read final block of data.
	ASSERT_NO_THROW(count = client2.read(buf, sizeof(buf), disconnected));
	ASSERT_EQ(disconnected, false);

	// Skip final \r\n.
	for (int i = 0; i < count - 2; i++) {
		if (chr == '9')
			chr = '0';
		else
			chr++;

		ASSERT_EQ(buf[i], chr);
	}
}

// Test that we can move RNG.
TEST(tls_test, rng_moveable)
{
	// This looks like it doesn't do much, but libasan will check to ensure
	// frees occur correctly.

	// Move-assign.
	janus::tls::rng rng1;
	janus::tls::rng rng2 = std::move(rng1);
	EXPECT_FALSE(rng2.seeded());

	// Move ctor.
	janus::tls::rng rng3;
	janus::tls::rng rng4(std::move(rng3));
	EXPECT_FALSE(rng4.seeded());

	// Move-assign seeded.
	janus::tls::rng rng5;
	rng5.seed();
	janus::tls::rng rng6 = std::move(rng5);
	EXPECT_TRUE(rng6.seeded());

	// Move ctor seeded.
	janus::tls::rng rng7;
	rng7.seed();
	janus::tls::rng rng8(std::move(rng7));
	EXPECT_TRUE(rng8.seeded());
}

// Test that we can move certs.
TEST(tls_test, certs_moveable)
{
	// This looks like it doesn't do much, but libasan will check to ensure
	// frees occur correctly.

	// Move-assign.
	janus::tls::certs certs1;
	janus::tls::certs certs2 = std::move(certs1);
	EXPECT_FALSE(certs2.loaded());
	EXPECT_FALSE(certs2.self_signed());

	// Move ctor.
	janus::tls::certs certs3;
	janus::tls::certs certs4(std::move(certs3));
	EXPECT_FALSE(certs4.loaded());
	EXPECT_FALSE(certs4.self_signed());

	// Move-assign, loaded.
	janus::tls::certs certs5;
	certs5.load();
	janus::tls::certs certs6 = std::move(certs5);
	EXPECT_TRUE(certs6.loaded());
	EXPECT_FALSE(certs6.self_signed());

	// Move ctor, loaded.
	janus::tls::certs certs7;
	certs7.load();
	janus::tls::certs certs8(std::move(certs7));
	EXPECT_TRUE(certs8.loaded());
	EXPECT_FALSE(certs8.self_signed());

	// Move-assign, loaded, self-signed.
	janus::tls::certs certs9;
	certs9.load("../test/test-cert/client-2048.crt", "../test/test-cert/client-2048.key");
	janus::tls::certs certs10 = std::move(certs9);
	EXPECT_TRUE(certs10.loaded());
	EXPECT_TRUE(certs10.self_signed());

	// Move ctor, loaded, self-signed.
	janus::tls::certs certs11;
	certs11.load("../test/test-cert/client-2048.crt", "../test/test-cert/client-2048.key");
	janus::tls::certs certs12(std::move(certs11));
	EXPECT_TRUE(certs12.loaded());
	EXPECT_TRUE(certs12.self_signed());
}
} // namespace
