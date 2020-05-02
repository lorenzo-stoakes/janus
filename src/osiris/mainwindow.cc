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
