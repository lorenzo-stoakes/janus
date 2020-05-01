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
