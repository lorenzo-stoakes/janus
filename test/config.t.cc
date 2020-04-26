#include "janus.hh"

#include <cstdlib>
#include <gtest/gtest.h>

namespace
{
// Test that we can read configuration files correctly.
TEST(config_test, parse)
{
	janus::config config1 = janus::parse_config("../test/test-config/config1.json");
	EXPECT_STREQ(config1.username.c_str(), "barrycunslow");
	EXPECT_STREQ(config1.password.c_str(), "password");
	EXPECT_STREQ(config1.app_key.c_str(), "12345");
	// This will be prefixed with config directory.
	EXPECT_STREQ(config1.cert_path.c_str(), "../test/test-config/client-2048.crt");
	EXPECT_STREQ(config1.key_path.c_str(), "../test/test-config/client-2048.key");

	janus::config config2 = janus::parse_config("../test/test-config/config2.json");
	EXPECT_STREQ(config2.username.c_str(), "barrycunslow");
	EXPECT_STREQ(config2.password.c_str(), "password");
	EXPECT_STREQ(config2.app_key.c_str(), "12345");
	// Absolute paths specified in config so nothing prefixed.
	EXPECT_STREQ(config2.cert_path.c_str(), "/home/barrycunslow/client-2048.crt");
	EXPECT_STREQ(config2.key_path.c_str(), "/home/barrycunslow/client-2048.key");

	std::string default_path = janus::internal::get_default_config_path();
	std::string expected_default_path = std::string(::getenv("HOME")) + "/.janus/config.json";
	EXPECT_EQ(default_path, expected_default_path);
}
} // namespace
