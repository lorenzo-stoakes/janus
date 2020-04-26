#include "janus.hh"

#include "sajson.hh"
#include <cstdlib>
#include <fstream>
#include <libgen.h>
#include <stdexcept>
#include <string>

namespace janus
{
// Extract the directory name from a path. This uses some horrid const cast
// allocation stuff but since we only read this config once it's tolerable.
static auto extract_dir_name(const std::string& path) -> std::string
{
	std::string copy = path;

	// Kind of horrible but we need to be able to modify this. We are taking
	// a copy of the string.
	char* buf = const_cast<char*>(copy.data());
	char* ret = ::dirname(buf);

	return std::string(ret);
}

// If this is a relative path we need to prepend with appropriate absolute
// directory.
static void normalise_path(const std::string& dir_name, std::string& path)
{
	if (path[0] == '/')
		return;

	path = dir_name + "/" + path;
}

static void parse_config_json(const std::string& path, std::string& json, config& config)
{
	char* buf = const_cast<char*>(json.data());

	sajson::document doc = janus::internal::parse_json(path.c_str(), buf, json.size());
	const sajson::value& root = doc.get_root();

	config.username = root.get_value_of_key(sajson::literal("username")).as_cstring();
	config.password = root.get_value_of_key(sajson::literal("password")).as_cstring();
	config.app_key = root.get_value_of_key(sajson::literal("app_key")).as_cstring();
	config.cert_path = root.get_value_of_key(sajson::literal("cert_path")).as_cstring();
	config.key_path = root.get_value_of_key(sajson::literal("key_path")).as_cstring();

	std::string dir_name = extract_dir_name(path);
	normalise_path(dir_name, config.cert_path);
	normalise_path(dir_name, config.key_path);
}

auto internal::get_default_config_path() -> std::string
{
	char* homedir = ::getenv("HOME");
	if (homedir == nullptr)
		throw std::runtime_error("Cannot determine home directory?");

	return std::string(homedir) + "/" + DEFAULT_CONFIG_PATH;
}

auto parse_config(const std::string& path) -> config
{
	config ret;

	std::string config_path;
	if (path == "")
		config_path = internal::get_default_config_path();
	else
		config_path = path;

	std::string content;
	if (auto stream = std::ifstream(config_path)) {
		content = std::string(std::istreambuf_iterator<char>(stream),
				      std::istreambuf_iterator<char>());
	} else {
		throw std::runtime_error(std::string("Cannot open ") + config_path);
	}

	parse_config_json(config_path, content, ret);
	return ret;
}
} // namespace janus
