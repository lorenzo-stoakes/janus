#include "janus.hh"

#include "maincontroller.hh"

#include <QtWidgets>
#include <ctime>
#include <stdexcept>
#include <string>

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
	// Not currently implemented so hide.
	_view->plLabel->hide();
	_view->hedgeAllButton->hide();

	for (uint64_t i = 0; i < NUM_DISPLAYED_RUNNERS; i++) {
		_ladders[i].set(_view, i);
		_ladders[i].init();
	}

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
		// fallthrough
	case update_level::RUNNERS:
		for (uint64_t i = 0; i < NUM_DISPLAYED_RUNNERS; i++) {
			_ladders[i].clear();
		}
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

void runner_ladder_ui::set(Ui::MainWindow* view, size_t index)
{
	if (index > NUM_DISPLAYED_RUNNERS)
		throw std::runtime_error(std::string("Invalid runner index ") +
					 std::to_string(index));

	switch (index) {
	case 0:
		table = view->ladder0TableWidget;
		combo = view->runner0ComboBox;
		traded_vol_label = view->runnerTradedVol0Label;
		traded_vol_sec_label = view->runnerTradedVolSec0Label;
		ltp_label = view->ltp0Label;
		status_frame = view->statusFrame0;
		tv_status_frame = view->tvStatusFrame0;
		break;
	case 1:
		table = view->ladder1TableWidget;
		combo = view->runner1ComboBox;
		traded_vol_label = view->runnerTradedVol1Label;
		traded_vol_sec_label = view->runnerTradedVolSec1Label;
		ltp_label = view->ltp1Label;
		status_frame = view->statusFrame1;
		tv_status_frame = view->tvStatusFrame1;
		break;
	case 2:
		table = view->ladder2TableWidget;
		combo = view->runner2ComboBox;
		traded_vol_label = view->runnerTradedVol2Label;
		traded_vol_sec_label = view->runnerTradedVolSec2Label;
		ltp_label = view->ltp2Label;
		status_frame = view->statusFrame2;
		tv_status_frame = view->tvStatusFrame2;
		break;
	case 3:
		table = view->ladder3TableWidget;
		combo = view->runner3ComboBox;
		traded_vol_label = view->runnerTradedVol3Label;
		traded_vol_sec_label = view->runnerTradedVolSec3Label;
		ltp_label = view->ltp3Label;
		status_frame = view->statusFrame3;
		tv_status_frame = view->tvStatusFrame3;
		break;
	}
}

void runner_ladder_ui::init()
{
	QPalette palette = table->palette();
	palette.setBrush(QPalette::Highlight, QBrush(Qt::white));
	palette.setBrush(QPalette::HighlightedText, QBrush(Qt::black));
	table->setPalette(palette);
	table->setFocusPolicy(Qt::NoFocus);
}

void runner_ladder_ui::clear()
{
	table->clear();

	combo->clear();
	traded_vol_label->setText("");
	traded_vol_sec_label->setText("");
	ltp_label->setText("");
	status_frame->setStyleSheet("");
	tv_status_frame->setStyleSheet("");
}
