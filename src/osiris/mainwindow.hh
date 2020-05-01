#pragma once

#include <QMainWindow>

#include "maincontroller.hh"

namespace Ui
{
class MainWindow;
}

class MainWindow : public QMainWindow
{
	Q_OBJECT

public:
	explicit MainWindow(main_controller& controller, QWidget* parent = nullptr);
	~MainWindow();

	// Get underlying UI object.
	auto ui() -> Ui::MainWindow*
	{
		return _ui;
	}

private slots:
	void on_raceDateSelecter_selectionChanged();

private:
	Ui::MainWindow* _ui;
	main_controller& _controller;
};
