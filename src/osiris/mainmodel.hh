#pragma once

#include "janus.hh"

#include <cstdint>
#include <unordered_map>
#include <vector>

// Represents the underlying state of Osiris.
class main_model
{
public:
	static constexpr uint64_t META_MAX_BYTES = 250'000'000;

	main_model();

	// Load config, metadata.
	void load();

	// Get all meta views stored within model.
	auto meta_views() const -> const std::vector<janus::meta_view>&
	{
		return _meta_views;
	}

	// Get meta views by days since epoch.
	auto meta_views_by_day() const
		-> const std::unordered_map<uint64_t, std::vector<janus::meta_view*>>&
	{
		return _meta_by_day;
	}

	// Get meta views for the specified day in ms since epoch.
	auto get_views_on_day(uint64_t ms) -> const std::vector<janus::meta_view*>&;

	// Get market on specified day at specified index.
	auto get_market_at(uint64_t ms, int index) -> janus::meta_view*;

private:
	janus::config _config;
	janus::dynamic_buffer _meta_dyn_buf;
	std::vector<janus::meta_view> _meta_views;
	std::unordered_map<uint64_t, std::vector<janus::meta_view*>> _meta_by_day;

	void populate_meta_by_day();
};
