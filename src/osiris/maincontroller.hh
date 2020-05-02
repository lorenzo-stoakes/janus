#pragma once

#include "mainmodel.hh"
#include "ui_mainwindow.h"

#include <cstdint>

enum class update_level
{
	FULL,
	MARKET_LIST,
	MARKET,
};

class main_controller
{
public:
	explicit main_controller(main_model& model) : _model{model}, _view{nullptr} {}

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

private:
	// Populate date selector, setting bold where data is available.
	void populate_dates();

	main_model& _model;
	Ui::MainWindow* _view;
};
