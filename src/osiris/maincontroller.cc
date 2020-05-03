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

	janus::betfair::runner* runner;

	for (uint64_t i = 0; i < runners.size(); i++) {
		runner = &runners[i];
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
	// Not currently implemented so hide.
	_view->plLabel->hide();
	_view->hedgeAllButton->hide();

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

		_view->marketNameLabel->setText("");
		_view->postLabel->setText("");
		_view->statusLabel->setText("");
		_view->inplayLabel->setText("");
		_view->nowLabel->setText("");
		_view->tradedVolLabel->setText("");

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
		combo->addItem(QString::fromUtf8(name.data(), name.size()));
	}

	// By default when setting the runner combo the index we're looking at
	// is also the index we set.
	combo->setCurrentIndex(index);

	_setting_up_combos = false;
}

void main_controller::apply_until_next_index()
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
			if (first) {
				_curr_timestamp = janus::get_update_timestamp(u);
			} else {
				_next_timestamp = janus::get_update_timestamp(u);
				// Unread the timestamp so we can read it next
				// time.
				dyn_buf.unread<janus::update>();
				_curr_index++;
				return;
			}
		}
		try {
			_curr_universe.apply_update(u);
		} catch (janus::universe_apply_error& e) {
			std::cerr << "ERROR parsing: " << e.what() << " aborting." << std::endl;
			return;
		}
		first = false;
	}
}

void main_controller::get_first_update()
{
	// The first timestamp precedes any existing data, so just consume it.
	apply_until_next_index();
	// This moves the index to the point where we are actually reading data.
	apply_until_next_index();
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

	if (runner->state() != janus::betfair::runner_state::REMOVED && runner->traded_vol() >= 1) {
		uint64_t ltp_index = runner->ltp();
		double ltp = janus::betfair::price_range::index_to_price(ltp_index);
		ladder_ui.ltp_label->setText(QString::number(ltp, 'g', 10));

		// Highlight just traded price in the ladder.
		uint64_t table_index = janus::betfair::NUM_PRICES - ltp_index - 1;
		QTableWidgetItem* price_item = ladder_ui.table->item(table_index, PRICE_COL);
		price_item->setBackground(PRICE_HIGHLIGHT_COLOUR);

	} else {
		ladder_ui.removed_label->setVisible(true);
	}

	janus::betfair::ladder& ladder = runner->ladder();
	QTableWidget* table = ladder_ui.table;
	for (uint64_t price_index = 0; price_index < janus::betfair::NUM_PRICES; price_index++) {
		uint64_t table_index = janus::betfair::NUM_PRICES - price_index - 1;

		bool back = true;
		double unmatched = ladder.unmatched(price_index);
		if (unmatched < 0) {
			back = false;
			unmatched = -unmatched;
		}
		if (unmatched >= 1) {
			if (back) {
				QTableWidgetItem* item = table->item(table_index, BACK_COL);
				item->setBackground(BACK_VOL_BG_COLOUR);
				item->setText(QString::number(static_cast<int>(unmatched)));
			} else {
				QTableWidgetItem* item = table->item(table_index, LAY_COL);
				item->setBackground(LAY_VOL_BG_COLOUR);
				item->setText(QString::number(static_cast<int>(unmatched)));
			}
		}

		double matched = ladder.matched(price_index);
		if (matched >= 1) {
			QTableWidgetItem* item = table->item(table_index, TRADED_COL);
			item->setText(QString::number(static_cast<int>(matched)));
		}
	}
}

// Update dynamic market fields.
void main_controller::update_market_dynamic()
{
	janus::betfair::market& market = _curr_universe.markets()[0];
	uint64_t timestamp = market.last_timestamp();
	std::string now = janus::local_epoch_ms_to_string(timestamp);
	_view->nowLabel->setText(QString::fromStdString(now));

	uint64_t start_timestamp = _curr_meta->market_start_timestamp();
	std::string start = janus::local_epoch_ms_to_string(start_timestamp);
	_view->postLabel->setText(QString::fromStdString(start));

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

	// Work out how many indexes of timestamps we have to iterate through.
	auto indexes = janus::index_market_updates(_model.update_dyn_buf(), _num_market_updates);
	_num_indexes = indexes.size();

	_view->marketNameLabel->setText(QString::fromStdString(title));

	auto& runners = _curr_meta->runners();
	for (uint64_t i = 0; i < runners.size(); i++) {
		if (i < NUM_DISPLAYED_RUNNERS) {
			_visible_runner_indexes[i] = i;

			populate_runner_combo(runners, i);
		}
	}

	get_first_update();
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
		apply_until_next_index();
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
		return;
	}

	if (state) {
		_playing = true;
		_playback_timer->start(_next_timestamp - _curr_timestamp);

	} else {
		_playing = false;
		_playback_timer->stop();
	}
}

void main_controller::timer_tick()
{
	_playback_timer->stop();

	if (!_playing || _curr_meta == nullptr)
		return;

	apply_until_next_index();

	_playback_timer->start(_next_timestamp - _curr_timestamp);

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
	if (clear_combo)
		combo->clear();

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
