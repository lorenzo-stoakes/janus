#include "janus.hh"

#include "maincontroller.hh"

#include <QtWidgets>
#include <ctime>

void main_controller::populate_dates()
{
	QTextCharFormat format;
	format.setFontWeight(QFont::Bold);

	for (auto& [days, views] : _model.meta_views_by_day()) {
		auto timeval = static_cast<std::time_t>(days * janus::MS_PER_DAY / 1000);
		QDateTime date_time = QDateTime::fromTime_t(timeval);
		QDate date = date_time.date();

		_view->raceDateSelecter->setDateTextFormat(date, format);
	}
}

void main_controller::init()
{
	populate_dates();
	select_date(_view->raceDateSelecter->selectedDate());
}

void main_controller::select_date(QDate date)
{
	QDateTime date_time = date.startOfDay();
	uint64_t ms = date_time.toMSecsSinceEpoch();

	_view->raceListWidget->clear();
	for (auto& view : _model.get_views_on_day(ms)) {
		std::string descr = view->describe();
		_view->raceListWidget->addItem(QString::fromStdString(descr));
	}
}
