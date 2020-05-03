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
	void on_raceListWidget_currentRowChanged(int row);
	void on_timeHorizontalSlider_sliderMoved(int pos);
	void on_follow0CheckBox_toggled(bool state);
	void on_follow1CheckBox_toggled(bool state);
	void on_follow2CheckBox_toggled(bool state);
	void on_follow3CheckBox_toggled(bool state);

private:
	Ui::MainWindow* _ui;
	main_controller& _controller;
};
