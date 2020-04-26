#pragma once

#include <string>

namespace janus
{
// Default location of configuration file (relative to home directory).
static constexpr const char* DEFAULT_CONFIG_PATH = ".janus/config.json";

// Represents configuration settings.
// We tolerate allocations here because this will only be read once.
struct config
{
	std::string username;
	std::string password;
	std::string app_key;
	std::string cert_path;
	std::string key_path;
};

namespace internal
{
// Retrieve default absolute path for the config file at ~/.janus/config.json.
auto get_default_config_path() -> std::string;
} // namespace internal

// Parse config file, if not specified defaults to ~/.janus/config.json. If
// relative, cert and key paths are relative to the location of the config.json
// file.
auto parse_config(const std::string& path = "") -> config;
} // namespace janus
