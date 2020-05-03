#include "maincontroller.hh"
#include "mainmodel.hh"
#include "mainwindow.hh"

#include <QApplication>

auto main(int argc, char* argv[]) -> int
{
	QApplication app(argc, argv);

	main_model model;
	model.load();
	main_controller controller(model);
	MainWindow window(controller); // We use camel case to avoid weirdness with auto-gen.
	controller.set_view(window.ui());
	controller.set_playback_timer(window.playback_timer());
	controller.init();

	window.show();

	return QApplication::exec();
}
