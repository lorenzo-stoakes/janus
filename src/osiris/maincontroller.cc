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
	clear(update_level::FULL);
	populate_dates();
	select_date(_view->raceDateSelecter->selectedDate());
}

void main_controller::clear(update_level level)
{
	switch (level) {
	case update_level::FULL:
		// fallthrough
	case update_level::MARKET_LIST:
		_view->raceListWidget->clear();
		// fallthrough
	case update_level::MARKET:
		_view->marketNameLabel->setText("");
		_view->postLabel->setText("");
		_view->inplayLabel->setText("");
		_view->nowLabel->setText("");
		_view->tradedVolLabel->setText("");
		_view->tradedPerSecVolLabel->setText("");
		break;
	}
}

void main_controller::select_date(QDate date)
{
	clear(update_level::MARKET_LIST);

	QDateTime date_time = date.startOfDay();
	uint64_t ms = date_time.toMSecsSinceEpoch();
	for (auto& view : _model.get_views_on_day(ms)) {
		std::string descr = view->describe();
		_view->raceListWidget->addItem(QString::fromStdString(descr));
	}
}
