#include "mainwindow.hh"
#include "ui_mainwindow.h"

MainWindow::MainWindow(main_controller& controller, QWidget* parent)
	: QMainWindow(parent), _ui(new Ui::MainWindow), _controller{controller}
{
	_ui->setupUi(this);
}

MainWindow::~MainWindow()
{
	delete _ui;
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
