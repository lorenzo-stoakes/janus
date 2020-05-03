#include "mainwindow.hh"
#include "ui_mainwindow.h"

#include <iostream>

MainWindow::MainWindow(main_controller& controller, QWidget* parent)
	: QMainWindow(parent),
	  _ui(new Ui::MainWindow),
	  _controller{controller},
	  _playback_timer{new QTimer(this)}
{
	_ui->setupUi(this);

	// TODO(lorenzo): Maybe handle this a different way? Getting error message:
	//   QMetaObject::connectSlotsByName: No matching signal for on__playback_timer_timeout()
	connect(_playback_timer, &QTimer::timeout, this,
		QOverload<>::of(&MainWindow::on__playback_timer_timeout));
}

MainWindow::~MainWindow()
{
	delete _ui;
	delete _playback_timer;
}

void MainWindow::on_actionExit_triggered()
{
	QApplication::quit();
}

void MainWindow::on_raceDateSelecter_selectionChanged()
{
	QDate date = _ui->raceDateSelecter->selectedDate();
	_controller.select_date(date);
}

void MainWindow::on_raceListWidget_currentRowChanged(int row)
{
	_controller.select_market(row);
}

void MainWindow::on_timeHorizontalSlider_sliderMoved(int pos)
{
	_controller.set_index(pos);
}

void MainWindow::on_follow0CheckBox_toggled(bool state)
{
	_controller.set_follow(0, state);
}

void MainWindow::on_follow1CheckBox_toggled(bool state)
{
	_controller.set_follow(1, state);
}

void MainWindow::on_follow2CheckBox_toggled(bool state)
{
	_controller.set_follow(2, state);
}

void MainWindow::on_follow3CheckBox_toggled(bool state)
{
	_controller.set_follow(3, state);
}

void MainWindow::on_runner0ComboBox_currentIndexChanged(int index)
{
	_controller.set_ladder_to_runner(0, index);
}

void MainWindow::on_runner1ComboBox_currentIndexChanged(int index)
{
	_controller.set_ladder_to_runner(1, index);
}

void MainWindow::on_runner2ComboBox_currentIndexChanged(int index)
{
	_controller.set_ladder_to_runner(2, index);
}

void MainWindow::on_runner3ComboBox_currentIndexChanged(int index)
{
	_controller.set_ladder_to_runner(3, index);
}

void MainWindow::on_playButton_clicked()
{
	_controller.toggle_play();
}

void MainWindow::on__playback_timer_timeout()
{
	_controller.timer_tick();
}

void MainWindow::on_crossmatchCheckBox_toggled(bool state)
{
	_controller.set_calc_virtual(state);
}
