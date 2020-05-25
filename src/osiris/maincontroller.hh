#pragma once

#include "janus.hh"
#include "mainmodel.hh"
#include "ui_mainwindow.h"

#include <array>
#include <cstdint>
#include <vector>

// Number of runners displayed at any one time.
static constexpr int NUM_DISPLAYED_RUNNERS = 4;

static constexpr int NUM_HISTORY_COLS = 20;

static constexpr int LAY_COL = NUM_HISTORY_COLS + 0;
static constexpr int PRICE_COL = NUM_HISTORY_COLS + 1;
static constexpr int BACK_COL = NUM_HISTORY_COLS + 2;
static constexpr int TRADED_COL = NUM_HISTORY_COLS + 3;
static constexpr int PL_COL = NUM_HISTORY_COLS + 4;
static constexpr int CANDLESTICK_COL_WIDTH = 3;
static constexpr int PL_COL_WIDTH = 45;

static constexpr QColor PRICE_BG_COLOUR = QColor(255, 255, 255);
static constexpr QColor PRICE_HIGHLIGHT_COLOUR = QColor(250, 146, 0); // Orange.

static constexpr QColor LAY_BG_COLOUR = QColor(246, 221, 228);
static constexpr QColor BACK_BG_COLOUR = QColor(191, 220, 245);

static const QColor LAY_VOL_BG_COLOUR = QColor(255, 142, 172);
static const QColor BACK_VOL_BG_COLOUR = QColor(39, 148, 228);

static constexpr const char* RED_FOREGROUND_STYLE = "color: rgb(255, 0, 0);";
static constexpr const char* GREEN_FOREGROUND_STYLE = "color: rgb(0, 128, 0);";

static constexpr const uint64_t PLAYBACK_INTERVAL_MS = 50;

// We increment a flash counter each time an LTP is written and the runner
// traded volume hasn't increased, once it's past this value then we clear
// it. This is used to flash traded price.
static constexpr const uint64_t MAX_FLASH_COUNT = 2;

enum class update_level
{
	FULL,
	MARKET_LIST,
	MARKET,
	RUNNERS,
};

// Represents a specific displayed runner ladder UI.
struct runner_ladder_ui
{
	runner_ladder_ui()
		: table{nullptr},
		  combo{nullptr},
		  traded_vol_label{nullptr},
		  removed_label{nullptr},
		  ltp_label{nullptr},
		  follow{true},
		  centre{true},
		  last_traded_vol{0},
		  flash_count{0}
	{
	}

	QTableWidget* table;
	QComboBox* combo;
	QLabel* traded_vol_label;
	QLabel* removed_label;
	QLabel* ltp_label;

	bool follow;
	bool centre;

	double last_traded_vol;
	uint64_t flash_count;

	// Set the runner ladder UI to the specified runner index.
	void set(Ui::MainWindow* _view, size_t index);

	// Initialise the runner ladder UI.
	void init(QString* price_strings);

	// Clear the contents of the runner ladder UI.
	void clear(QString* price_strings, bool clear_combo = false);
};

class main_controller
{
public:
	explicit main_controller(main_model& model)
		: _model{model},
		  _view{nullptr},
		  _playback_timer{nullptr},
		  _selected_market_index{-1},
		  _selected_date_ms{0},
		  _num_market_updates{0},
		  _num_indexes{0},
		  _curr_index{0},
		  _curr_timestamp{0},
		  _next_timestamp{0},
		  _last_timestamp{0},
		  _playback_timestamp{0},
		  _visible_runner_indexes{-1},
		  _ladders{},
		  _curr_meta{nullptr},
		  _setting_up_combos{false},
		  _playing{false},
		  _calc_virtual{false}
	{
		init_price_strings();
	}

	// Set the view. We want to be able to pass the controller to the view
	// so we pass the model in the ctor and the view here.
	void set_view(Ui::MainWindow* view)
	{
		_view = view;
	}

	// Set the playback timer.
	void set_playback_timer(QTimer* timer)
	{
		_playback_timer = timer;
	}

	// Populate the view with initial state.
	void init();

	// Obtain data for the specified date.
	void select_date(QDate date);

	// Select the market at the specified index in the market list.
	void select_market(int index);

	// Set the market index to the specified value.
	void set_index(int index);

	// Set whether the specified ladder should be followed or not.
	void set_follow(int index, bool state);

	// Set the specified ladder runner index.
	void set_ladder_to_runner(int ladder_index, int runner_index);

	// Toggle playback.
	void toggle_play();

	// Toggle playback to specified state.
	void toggle_play(bool state);

	// Tick timer.
	void timer_tick();

	// Determine whether we calculate virtual prices (cross-match).
	void set_calc_virtual(bool state);

private:
	main_model& _model;
	Ui::MainWindow* _view;
	QTimer* _playback_timer;
	int _selected_market_index;
	uint64_t _selected_date_ms;
	uint64_t _num_market_updates;
	uint64_t _num_indexes;
	uint64_t _curr_index;
	uint64_t _curr_timestamp;
	uint64_t _next_timestamp;
	uint64_t _last_timestamp;
	uint64_t _playback_timestamp;

	std::array<int, NUM_DISPLAYED_RUNNERS> _visible_runner_indexes;
	std::array<runner_ladder_ui, NUM_DISPLAYED_RUNNERS> _ladders;
	std::array<QString, janus::betfair::NUM_PRICES> _price_strings;

	janus::betfair::universe<1> _curr_universe;
	janus::meta_view* _curr_meta;

	bool _setting_up_combos;
	bool _playing;
	bool _calc_virtual;

	// Clear UI at specified update level.
	void clear(update_level level);

	// Populate date selector, setting bold where data is available.
	void populate_dates();

	// Populate each runner combo and set indexes to defaults.
	void populate_runner_combo(const std::vector<janus::runner_view>& runners, int index);

	// Clear lader for specified runner index, setting all formatting correctly.
	void clear_ladder(int index);

	// Initialise price strings.
	void init_price_strings();

	// Follow ladder if follow enabled.
	void follow_ladder(int index);

	// Get runner with the specific runner metadata.
	auto get_runner(const janus::runner_view& runner_meta) -> janus::betfair::runner*;

	// Apply updates until (but not including) the next timestamp. Returns
	// false if an error occurred.
	auto apply_until_next_index() -> bool;

	// Update the first block of updates and apply to the current
	// universe. Returns false if an error occurred.
	auto get_first_update() -> bool;

	// Update the visible runner at the specified index.
	void update_ladder(int index);

	// Update all ladders based on current universe and all market-specific
	// data.
	void update_market_dynamic();

	// Clear the runner LTP list.
	void clear_runner_ltp_list();

	// Update the runner LTP list.
	void update_runner_ltp_list();

	// Generate a virtual ladder for the specified runner.
	auto gen_virtual(const janus::runner_view& runner_meta) -> janus::betfair::ladder;
};
