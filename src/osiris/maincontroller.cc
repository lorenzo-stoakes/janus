#include "janus.hh"

#include "maincontroller.hh"

#include <QtWidgets>
#include <ctime>
#include <iostream>
#include <stdexcept>
#include <string>
#include <string_view>

// Create a read-only table item for insertion into a table.
static auto make_readonly_table_item() -> QTableWidgetItem*
{
	QTableWidgetItem* item = new QTableWidgetItem();

	item->setTextAlignment(Qt::AlignCenter);
	item->setFlags(item->flags() ^ (Qt::ItemIsEditable | Qt::ItemIsSelectable));

	return item;
}

auto main_controller::get_runner(const janus::runner_view& runner_meta) -> janus::betfair::runner*
{
	janus::betfair::market& market = _curr_universe.markets()[0];
	auto& runners = market.runners();

	for (uint64_t i = 0; i < runners.size(); i++) {
		janus::betfair::runner* runner = &runners[i];
		if (runner->id() == runner_meta.id())
			return runner;
	}

	return nullptr;
}

void main_controller::populate_dates()
{
	QTextCharFormat format;
	format.setFontWeight(QFont::Bold);

	for (auto& [days, views] : _model.meta_views_by_day()) {
		auto timeval = static_cast<std::time_t>(days * janus::MS_PER_DAY / 1000);
		QDateTime date_time = QDateTime::fromTime_t(timeval);
		QDate date = date_time.date();

		_view->raceDateSelecter->setDateTextFormat(date, format);
	}
}

void main_controller::init_price_strings()
{
	for (uint64_t i = 0; i < janus::betfair::NUM_PRICES; i++) {
		double price = janus::betfair::price_range::index_to_price(
			janus::betfair::NUM_PRICES - i - 1);
		_price_strings[i] = QString::number(price, 'g', 10); // NOLINT: Not magical.
	}
}

void main_controller::init()
{
	for (uint64_t i = 0; i < NUM_DISPLAYED_RUNNERS; i++) {
		_ladders[i].set(_view, i);
		_ladders[i].init(&_price_strings[0]);
	}

	// Initialise runner LTP list.
	auto* table = _view->runnerLTPTableWidget;
	auto* header = table->horizontalHeader();
	header->setSectionResizeMode(0, QHeaderView::Stretch);
	table->sortItems(1, Qt::AscendingOrder);
	// Reduce LTP column size.
	table->setColumnWidth(1, 30); // NOLINT: Not magical.

	clear(update_level::FULL);
	populate_dates();
	select_date(_view->raceDateSelecter->selectedDate());
}

void main_controller::clear(update_level level)
{
	switch (level) {
	case update_level::FULL:
		// fallthrough
	case update_level::MARKET_LIST:
		_selected_market_index = -1;
		_selected_date_ms = 0;
		_view->raceListWidget->clear();
		// fallthrough
	case update_level::MARKET:
		toggle_play(false);

		_num_market_updates = 0;
		_num_indexes = 0;
		_curr_index = 0;
		_curr_universe.clear();
		_curr_meta = nullptr;
		_curr_timestamp = 0;
		_next_timestamp = 0;
		_last_timestamp = 0;
		_playback_timestamp = 0;

		_view->marketNameLabel->setText("");
		_view->postLabel->setText("");
		_view->lastLabel->setText("");
		_view->statusLabel->setText("");
		_view->inplayLabel->setText("");
		_view->nowLabel->setText("");
		_view->tradedVolLabel->setText("");
		_view->timestampLabel->setText("");

		_view->timeHorizontalSlider->setValue(0);
		_view->timeHorizontalSlider->setMinimum(0);
		_view->timeHorizontalSlider->setMaximum(0);

		// fallthrough
	case update_level::RUNNERS:
		_visible_runner_indexes.fill(-1);

		clear_runner_ltp_list();

		_setting_up_combos = true;
		for (uint64_t i = 0; i < NUM_DISPLAYED_RUNNERS; i++) {
			_ladders[i].clear(&_price_strings[0], true);
		}
		_setting_up_combos = false;
		break;
	}
}

void main_controller::select_date(QDate date)
{
	clear(update_level::MARKET_LIST);

	QDateTime date_time = date.startOfDay(Qt::UTC);
	uint64_t ms = date_time.toMSecsSinceEpoch();
	_selected_date_ms = ms;

	for (auto& view : _model.get_views_on_day(ms)) {
		std::string descr = view->describe();
		_view->raceListWidget->addItem(QString::fromStdString(descr));
	}
}

void main_controller::populate_runner_combo(const std::vector<janus::runner_view>& runners,
					    int index)
{
	_setting_up_combos = true;

	auto* combo = _ladders[index].combo;

	for (const auto& runner : runners) {
		std::string_view name = runner.name();
		std::string runner_text =
			std::string(name.data()) + " (" + std::to_string(runner.id()) + ")";

		combo->addItem(QString::fromStdString(runner_text));
	}

	// By default when setting the runner combo the index we're looking at
	// is also the index we set.
	combo->setCurrentIndex(index);

	_setting_up_combos = false;
}

auto main_controller::apply_until_next_index() -> bool
{
	janus::dynamic_buffer& dyn_buf = _model.update_dyn_buf();

	bool first = true;
	while (dyn_buf.read_offset() < dyn_buf.size()) {
		auto& u = dyn_buf.read<janus::update>();
		if (u.type == janus::update_type::TIMESTAMP) {
			// Getting to timestamp means we've consumed the
			// entirity of this set of updates for this timesliver.
			// timesliver.  If it's the first item in the stream
			// then we just consume it so we don't stall forever.
			uint64_t timestamp = janus::get_update_timestamp(u);
			if (first) {
				_curr_timestamp = timestamp;
			} else if (timestamp != _curr_timestamp) {
				_next_timestamp = timestamp;
				// Unread the timestamp so we can read it next
				// time.
				dyn_buf.unread<janus::update>();
				_curr_index++;
				return true;
			}
		}
		try {
			_curr_universe.apply_update(u);
		} catch (janus::universe_apply_error& e) {
			std::cerr << "ERROR parsing: " << e.what() << " aborting." << std::endl;
			return false;
		}
		first = false;
	}

	return false;
}

auto main_controller::get_first_update() -> bool
{
	// The first timestamp precedes any existing data, so just consume it.
	if (!apply_until_next_index())
		return false;
	// This moves the index to the point where we are actually reading data.
	if (!apply_until_next_index())
		return false;

	return true;
}

auto main_controller::gen_virtual(const janus::runner_view& runner_meta) -> janus::betfair::ladder
{
	janus::betfair::market& market = _curr_universe.markets()[0];
	auto& runners = market.runners();

	janus::betfair::ladder ladder;
	std::vector<janus::betfair::ladder> others;
	for (uint64_t i = 0; i < runners.size(); i++) {
		auto& runner = runners[i];
		if (runner.state() != janus::betfair::runner_state::ACTIVE)
			continue;

		if (runner.id() == runner_meta.id())
			ladder = runner.ladder();
		else
			others.push_back(runner.ladder());
	}

	janus::betfair::price_range range;
	return janus::betfair::gen_virt_ladder(range, ladder, others);
}

void main_controller::update_ladder(int ladder_index)
{
	int index = _visible_runner_indexes[ladder_index];
	if (index == -1)
		return;

	auto& runner_meta = _curr_meta->runners()[index];
	janus::betfair::runner* runner = get_runner(runner_meta);

	// Couldn't find runner?
	if (runner == nullptr) {
		std::cerr << "Unable to find runner " << runner_meta.id() << std::endl;
		return;
	}

	auto& ladder_ui = _ladders[ladder_index];
	ladder_ui.clear(&_price_strings[0]);

	int traded_vol = static_cast<int>(runner->traded_vol());
	ladder_ui.traded_vol_label->setText(QString::number(traded_vol));

	bool is_removed = runner->state() == janus::betfair::runner_state::REMOVED;
	ladder_ui.removed_label->setVisible(false);
	if (is_removed) {
		ladder_ui.removed_label->setVisible(true);
	} else {
		uint64_t ltp_index = runner->ltp();
		double ltp = janus::betfair::price_range::index_to_price(ltp_index);
		ladder_ui.ltp_label->setText(QString::number(ltp, 'g', 10));

		// Highlight just traded price in the ladder.
		uint64_t table_index = janus::betfair::NUM_PRICES - ltp_index - 1;
		QTableWidgetItem* price_item = ladder_ui.table->item(table_index, PRICE_COL);
		if (ladder_ui.last_traded_vol != traded_vol) {
			price_item->setBackground(PRICE_HIGHLIGHT_COLOUR);
			ladder_ui.last_traded_vol = traded_vol;
			ladder_ui.flash_count = 0;
		} else {
			ladder_ui.flash_count++;

			if (ladder_ui.flash_count >= MAX_FLASH_COUNT)
				price_item->setBackground(PRICE_BG_COLOUR);
			else
				price_item->setBackground(PRICE_HIGHLIGHT_COLOUR);
		}
	}

	const janus::betfair::ladder& ladder =
		_calc_virtual ? gen_virtual(runner_meta) : runner->ladder();
	QTableWidget* table = ladder_ui.table;
	for (uint64_t price_index = 0; price_index < janus::betfair::NUM_PRICES; price_index++) {
		uint64_t table_index = janus::betfair::NUM_PRICES - price_index - 1;

		bool back = true;
		double unmatched = ladder.unmatched(price_index);
		if (unmatched < 0) {
			back = false;
			unmatched = -unmatched;
		}
		if (unmatched > 0) {
			QTableWidgetItem* item;
			if (back) {
				item = table->item(table_index, BACK_COL);
				item->setBackground(BACK_VOL_BG_COLOUR);
			} else {
				item = table->item(table_index, LAY_COL);
				item->setBackground(LAY_VOL_BG_COLOUR);
			}

			if (unmatched < 1)
				item->setText(QString::number(unmatched, 'f', 2));
			else
				item->setText(QString::number(static_cast<int>(unmatched)));
		}

		double matched = ladder.matched(price_index);
		if (matched > 0) {
			QTableWidgetItem* item = table->item(table_index, TRADED_COL);
			if (matched < 1)
				item->setText(QString::number(matched, 'f', 2));
			else
				item->setText(QString::number(static_cast<int>(matched)));
		}
	}
}

// Update dynamic market fields.
void main_controller::update_market_dynamic()
{
	janus::betfair::market& market = _curr_universe.markets()[0];

	uint64_t timestamp;

	if (_playing)
		timestamp = _playback_timestamp;
	else
		timestamp = _curr_timestamp;
	std::string now = janus::local_epoch_ms_to_string(timestamp);

	_view->nowLabel->setText(QString::fromStdString(now));
	_view->timestampLabel->setText(QString::number(timestamp));

	uint64_t start_timestamp = _curr_meta->market_start_timestamp();
	std::string start = janus::local_epoch_ms_to_string(start_timestamp);
	_view->postLabel->setText(QString::fromStdString(start));

	std::string last = janus::local_epoch_ms_to_string(_last_timestamp);
	_view->lastLabel->setText(QString::fromStdString(last));

	std::string state;
	switch (market.state()) {
	case janus::betfair::market_state::OPEN:
		state = "OPEN";
		_view->statusLabel->setStyleSheet(GREEN_FOREGROUND_STYLE);
		break;
	case janus::betfair::market_state::CLOSED:
		state = "CLOSED";
		_view->statusLabel->setStyleSheet(RED_FOREGROUND_STYLE);
		break;
	case janus::betfair::market_state::SUSPENDED:
		state = "SUSPENDED";
		_view->statusLabel->setStyleSheet(RED_FOREGROUND_STYLE);
		break;
	default:
		state = "UNKNOWN?";
		_view->statusLabel->setStyleSheet(RED_FOREGROUND_STYLE);
		break;
	}
	_view->statusLabel->setText(QString::fromStdString(state));

	if (market.inplay()) {
		_view->inplayLabel->setText(QString::fromUtf8("INPLAY"));
		_view->inplayLabel->setStyleSheet(RED_FOREGROUND_STYLE);
	} else {
		_view->inplayLabel->setText(QString::fromUtf8("PRE"));
		_view->inplayLabel->setStyleSheet(GREEN_FOREGROUND_STYLE);
	}

	_view->tradedVolLabel->setText(QString::number(static_cast<int>(market.traded_vol())));

	for (uint64_t i = 0; i < NUM_DISPLAYED_RUNNERS; i++) {
		update_ladder(i);
		follow_ladder(i);
	}

	update_runner_ltp_list();
}

void main_controller::select_market(int index)
{
	if (index < 0)
		return;

	clear(update_level::MARKET);

	_selected_market_index = index;
	_curr_meta = _model.get_market_at(_selected_date_ms, index);
	std::string title =
		_curr_meta->describe() + " (" + std::to_string(_curr_meta->market_id()) + ")";

	_num_market_updates = _model.get_market_updates(_curr_meta->market_id());
	// Setup our market.
	_curr_universe.apply_update(janus::make_market_id_update(_curr_meta->market_id()));

	_view->marketNameLabel->setText(QString::fromStdString(title));

	auto& runners = _curr_meta->runners();
	for (uint64_t i = 0; i < runners.size(); i++) {
		if (i < NUM_DISPLAYED_RUNNERS) {
			_visible_runner_indexes[i] = i;

			populate_runner_combo(runners, i);
		}
	}

	if (_num_market_updates == 0)
		return;

	// Work out how many indexes of timestamps we have to iterate through.
	auto indexes = janus::index_market_updates(_model.update_dyn_buf(), _num_market_updates);
	_num_indexes = indexes.size();
	janus::update* ptr = reinterpret_cast<janus::update*>(_model.update_dyn_buf().data());
	uint64_t last_offset = indexes[_num_indexes - 1];
	_last_timestamp = janus::get_update_timestamp(ptr[last_offset]);

	if (!get_first_update())
		std::cerr << "Error in getting first update." << std::endl;

	_view->timeHorizontalSlider->setMinimum(_curr_index);
	_view->timeHorizontalSlider->setMaximum(_num_indexes - 1);

	update_market_dynamic();
}

void main_controller::set_index(int index)
{
	auto index_val = static_cast<uint64_t>(index);

	// We disable playback on index set.
	toggle_play(false);

	// If we're rewinding then we need to clear and re-run up to the index.
	// TODO(lorenzo): Make this more efficient.
	if (index_val < _curr_index) {
		_model.update_dyn_buf().reset_read();
		_curr_universe.clear();
		_curr_universe.apply_update(janus::make_market_id_update(_curr_meta->market_id()));
		_curr_index = 0;
		get_first_update();
	}

	while (_curr_index < index_val && _curr_index < _num_indexes) {
		if (!apply_until_next_index()) {
			std::cerr << "Error in apply next index, aborting." << std::endl;
			break;
		}
	}

	update_market_dynamic();
}

void main_controller::follow_ladder(int index)
{
	int runner_index = _visible_runner_indexes[index];
	if (runner_index == -1)
		return;

	auto& ladder_ui = _ladders[index];
	if (!ladder_ui.follow || _curr_meta == nullptr)
		return;

	auto& runner_meta = _curr_meta->runners()[runner_index];
	auto* runner = get_runner(runner_meta);
	if (runner == nullptr) {
		std::cerr << "Unable to find runner " << runner_meta.id() << std::endl;
		return;
	}

	uint64_t ltp_index = runner->ltp();
	// The prices are shown in inverse order.
	uint64_t row = janus::betfair::NUM_PRICES - ltp_index - 1;

	auto* table = ladder_ui.table;

	// We centre on first follow, after that we just show visible to avoid stutter.
	if (ladder_ui.centre) {
		table->scrollToItem(table->item(row, LAY_COL), QAbstractItemView::PositionAtCenter);
		ladder_ui.centre = false;
	} else {
		table->scrollToItem(table->item(row, LAY_COL), QAbstractItemView::EnsureVisible);
	}
}

void main_controller::set_follow(int index, bool state)
{
	if (index >= NUM_DISPLAYED_RUNNERS)
		throw std::runtime_error(std::string("Unexpected set_follow() for ") +
					 std::to_string(index));

	_ladders[index].follow = state;
	// We centre on first follow.
	if (state)
		_ladders[index].centre = true;

	update_market_dynamic();
}

void main_controller::set_ladder_to_runner(int ladder_index, int runner_index)
{
	if (_setting_up_combos)
		return;

	if (ladder_index >= NUM_DISPLAYED_RUNNERS)
		throw std::runtime_error(std::string("Unexpected set_ladder_to_runner() for ") +
					 std::to_string(ladder_index));

	_visible_runner_indexes[ladder_index] = runner_index;
	update_ladder(ladder_index);
	if (_ladders[ladder_index].follow)
		_ladders[ladder_index].centre = true;
	follow_ladder(ladder_index);
}

// Clear the runner LTP list.
void main_controller::clear_runner_ltp_list()
{
	_view->runnerLTPTableWidget->clearContents();
	// TODO(lorenzo): Free items!!
	while (_view->runnerLTPTableWidget->rowCount() > 0) {
		_view->runnerLTPTableWidget->removeRow(0);
	}
}

// Update the runner LTP list.
void main_controller::update_runner_ltp_list()
{
	if (_curr_meta == nullptr)
		return;

	clear_runner_ltp_list();

	// TODO(lorenzo): Lookup runners more efficiently.
	auto& runner_metas = _curr_meta->runners();
	for (uint64_t i = 0; i < runner_metas.size(); i++) {
		auto& runner_meta = runner_metas[i];
		auto* runner = get_runner(runner_meta);
		if (runner == nullptr) {
			std::cerr << "Unable to find runner " << runner_meta.id() << std::endl;
			return;
		}

		auto* name_item = make_readonly_table_item();
		std::string_view name = runner_meta.name();
		name_item->setText(QString::fromUtf8(name.data(), name.size()));

		auto* ltp_item = make_readonly_table_item();
		if (runner->state() == janus::betfair::runner_state::REMOVED) {
			ltp_item->setText("NR");
		} else if (runner->traded_vol() < 1) {
			ltp_item->setText("NIL");
		} else {
			double ltp = janus::betfair::price_range::index_to_price(runner->ltp());
			ltp_item->setData(Qt::DisplayRole, ltp);
		}

		_view->runnerLTPTableWidget->insertRow(i);
		_view->runnerLTPTableWidget->setItem(i, 0, name_item);
		_view->runnerLTPTableWidget->setItem(i, 1, ltp_item);
	}
}

void main_controller::toggle_play()
{
	toggle_play(!_playing);
}

void main_controller::toggle_play(bool state)
{
	if (_curr_meta == nullptr) {
		_playback_timer->stop();
		_playback_timestamp = 0;
		return;
	}

	if (state) {
		_playback_timestamp = _curr_timestamp;
		_playing = true;
		_playback_timer->start(PLAYBACK_INTERVAL_MS);

	} else {
		_playing = false;
		_playback_timer->stop();
	}
}

void main_controller::timer_tick()
{
	if (!_playing || _curr_meta == nullptr)
		return;

	_playback_timestamp += PLAYBACK_INTERVAL_MS;

	while (_next_timestamp < _playback_timestamp && _curr_index < _num_indexes) {
		if (!apply_until_next_index()) {
			std::cerr << "Error in processing next message, timer stopped."
				  << std::endl;
			_playback_timer->stop();
			_playing = false;
			break;
		}
	}

	_view->timeHorizontalSlider->setValue(_curr_index);
	update_market_dynamic();
}

void main_controller::rewind()
{
	if (_playing || _curr_index == 0 || _curr_meta == nullptr)
		return;

	set_index(_curr_index - 1);

	_view->timeHorizontalSlider->setValue(_curr_index);
	update_market_dynamic();
}

void main_controller::fastforward()
{
	if (_playing || _curr_index >= _num_indexes - 1 || _curr_meta == nullptr)
		return;

	set_index(_curr_index + 1);

	_view->timeHorizontalSlider->setValue(_curr_index);
	update_market_dynamic();
}

void main_controller::set_calc_virtual(bool state)
{
	_calc_virtual = state;

	if (_curr_meta != nullptr)
		update_market_dynamic();
}

void runner_ladder_ui::set(Ui::MainWindow* view, size_t index)
{
	if (index > NUM_DISPLAYED_RUNNERS)
		throw std::runtime_error(std::string("Invalid runner index ") +
					 std::to_string(index));

	switch (index) {
	case 0:
		table = view->ladder0TableWidget;
		combo = view->runner0ComboBox;
		traded_vol_label = view->runnerTradedVol0Label;
		removed_label = view->removed0Label;
		ltp_label = view->ltp0Label;
		break;
	case 1:
		table = view->ladder1TableWidget;
		combo = view->runner1ComboBox;
		traded_vol_label = view->runnerTradedVol1Label;
		removed_label = view->removed1Label;
		ltp_label = view->ltp1Label;
		break;
	case 2:
		table = view->ladder2TableWidget;
		combo = view->runner2ComboBox;
		traded_vol_label = view->runnerTradedVol2Label;
		removed_label = view->removed2Label;
		ltp_label = view->ltp2Label;
		break;
	case 3:
		table = view->ladder3TableWidget;
		combo = view->runner3ComboBox;
		traded_vol_label = view->runnerTradedVol3Label;
		removed_label = view->removed3Label;
		ltp_label = view->ltp3Label;
		break;
	}
}

void runner_ladder_ui::init(QString* price_strings)
{
	QPalette palette = table->palette();
	palette.setBrush(QPalette::Highlight, QBrush(Qt::white));
	palette.setBrush(QPalette::HighlightedText, QBrush(Qt::black));
	table->setPalette(palette);
	table->setFocusPolicy(Qt::NoFocus);

	// Set column widths.
	for (int col = 0; col < table->columnCount(); col++) {
		if (col < NUM_HISTORY_COLS)
			table->setColumnWidth(col, CANDLESTICK_COL_WIDTH);
		else if (col == PL_COL)
			table->setColumnWidth(col, PL_COL_WIDTH);
	}

	// Set table items.
	for (int row = 0; row < table->rowCount(); row++) {
		for (int col = 0; col < table->columnCount(); col++) {
			auto* item = make_readonly_table_item();

			if (col == PRICE_COL) {
				QFont font = item->font();
				font.setWeight(QFont::Bold);
				item->setFont(font);
				item->setText(price_strings[row]);
			}

			table->setItem(row, col, item);
		}
	}
}

void runner_ladder_ui::clear(QString* price_strings, bool clear_combo)
{
	if (clear_combo) {
		combo->clear();
		last_traded_vol = 0;
		flash_count = 0;
	}

	traded_vol_label->setText("");
	removed_label->setVisible(false);
	ltp_label->setText("");

	for (int row = 0; row < table->rowCount(); row++) {
		for (int col = 0; col < table->columnCount(); col++) {
			QTableWidgetItem* item = table->item(row, col);

			// We only want to keep the text in the price column.
			if (col != PRICE_COL)
				item->setText("");

			// Clear history columns.
			if (col < NUM_HISTORY_COLS) {
				item->setBackground(Qt::white);
				continue;
			}

			switch (col) {
			case LAY_COL:
				item->setBackground(LAY_BG_COLOUR);
				break;
			case PRICE_COL:
				item->setBackground(PRICE_BG_COLOUR);
				item->setText(price_strings[row]);
				break;
			case BACK_COL:
				item->setBackground(BACK_BG_COLOUR);
				break;
			case TRADED_COL:
				item->setBackground(Qt::white);
				break;
			case PL_COL:
				// No styling required.
				break;
			}
		}
	}
}
