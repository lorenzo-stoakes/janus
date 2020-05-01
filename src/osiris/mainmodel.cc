#include "mainmodel.hh"

main_model::main_model() : _meta_dyn_buf{META_MAX_BYTES} {}

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
