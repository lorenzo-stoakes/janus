#pragma once

#include "janus.hh"
#include "mainmodel.hh"
#include "ui_mainwindow.h"

#include <array>
#include <cstdint>

// Number of runners displayed at any one time.
static constexpr uint64_t NUM_DISPLAYED_RUNNERS = 4;

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
	QTableWidget* table;
	QComboBox* combo;
	QLabel* traded_vol_label;
	QLabel* traded_vol_sec_label;
	QLabel* ltp_label;

	QFrame* status_frame;
	QFrame* tv_status_frame;

	// Set the runner ladder UI to the specified runner index.
	void set(Ui::MainWindow* _view, size_t index);

	// Initialise the runner ladder UI.
	void init();

	// Clear the contents of the runner ladder UI.
	void clear();
};

class main_controller
{
public:
	explicit main_controller(main_model& model)
		: _model{model},
		  _view{nullptr},
		  _selected_market_index{-1},
		  _selected_date_ms{0},
		  _visible_runner_indexes{-1}
	{
	}

	// Set the view. We want to be able to pass the controller to the view
	// so we pass the model in the ctor and the view here.
	void set_view(Ui::MainWindow* view)
	{
		_view = view;
	}

	// Populate the view with initial state.
	void init();

	// Clear UI at specified update level.
	void clear(update_level level);

	// Obtain data for the specified date.
	void select_date(QDate date);

	// Select the market at the specified index in the market list.
	void select_market(int index);

private:
	main_model& _model;
	Ui::MainWindow* _view;
	int _selected_market_index;
	uint64_t _selected_date_ms;

	std::array<int, NUM_DISPLAYED_RUNNERS> _visible_runner_indexes;
	std::array<runner_ladder_ui, NUM_DISPLAYED_RUNNERS> _ladders;

	// Populate date selector, setting bold where data is available.
	void populate_dates();

	void populate_runner_combo(const std::vector<janus::runner_view>& runners, int index);
};
