#include "mainmodel.hh"

#include <vector>

main_model::main_model() : _meta_dyn_buf{META_MAX_BYTES}, _update_dyn_buf{UPDATE_MAX_BYTES} {}

void main_model::load()
{
	_config = janus::parse_config();
	_meta_views = janus::read_all_metadata(_config, _meta_dyn_buf);
	populate_meta_by_day();
}

void main_model::populate_meta_by_day()
{
	for (auto& view : _meta_views) {
		uint64_t ms = view.market_start_timestamp();
		uint64_t days = ms / janus::MS_PER_DAY;
		_meta_by_day[days].push_back(&view);
	}
}

auto main_model::get_views_on_day(uint64_t ms) -> const std::vector<janus::meta_view*>&
{
	uint64_t days = ms / janus::MS_PER_DAY;
	return _meta_by_day[days];
}

auto main_model::get_market_at(uint64_t ms, int index) -> janus::meta_view*
{
	const auto& on_day = get_views_on_day(ms);
	return on_day[index];
}

auto main_model::get_market_updates(uint64_t id) -> uint64_t
{
	_update_dyn_buf.reset();
	return janus::read_market_updates(_config, _update_dyn_buf, id);
}
