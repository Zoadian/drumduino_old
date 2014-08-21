#include "stdafx.h"
#include "drumduino.h"

#include "porttab.h"

size_t mapChannels(size_t channel)
{
	return channel;
	//size_t port = channel / 8;
	//size_t chan = channel % 8;

	//const size_t pinMapping[8] = {2, 4, 1, 6, 0, 7, 3, 5};
	//return port * CHAN_CNT + pinMapping[chan] - 1;
}


drumduino::drumduino(QWidget* parent)
	: QMainWindow(parent)
	, _lasttime(QDateTime::currentMSecsSinceEpoch())
{
	ui.setupUi(this);

	for(auto i = 0; i < PORT_CNT; ++i) {
		ui.tabWidget->addTab(new PortTab(), "Port_" + QString::number(i));
	}

	for(auto i = 0; i < PORT_CNT * CHAN_CNT; ++i) {
		_plots.push_back(new QCustomPlot(ui.tabWidget->widget(i / 8)));
		_plots.back()->addGraph();

		_plots.back()->xAxis->setRange(0, 1024);
		_plots.back()->yAxis->setRange(0, 127);
		_plots.back()->resize(1100, 100);
		_plots.back()->move(0, i % 8 * 100);
	}


	_serial = std::make_shared<Serial>(L"COM3", 115200);
	_midiOut = std::make_shared<MidiOut>(1);

	{
		QTimer* timer = new QTimer(this);
		_lasttime = QDateTime::currentMSecsSinceEpoch();
		connect(timer, &QTimer::timeout, this, &drumduino::serialRead);
		timer->start(0);
	}


	//{
	//  QTimer* timer = new QTimer(this);
	//  connect(timer, &QTimer::timeout, this, &drumduino::updateGraph);
	//  timer->start(1000);
	//}
}

drumduino::~drumduino()
{

}

void drumduino::serialRead()
{
AGAIN:
	auto available = _serial->available();

	if(available < 1 + PORT_CNT * CHAN_CNT) {
		return;
	}

	byte sentinel;
	_serial->readBytes(&sentinel, 1);

	if(sentinel != 0xff) {
		goto AGAIN;
	}

	//now we have a full frame

	std::array<byte, PORT_CNT* CHAN_CNT> frame;
	_serial->readBytes(frame.data(), frame.size());

	auto currentIndex = _currentFrame % 1024;
	handleFrame(frame, currentIndex);


	//for(size_t i = 0; i < PORT_CNT * CHAN_CNT; ++i) {
	//  _frameBuffer[mapChannels(i)][currentIndex] = frame[i];
	//}

	++_currentFrame;

	//auto now = QDateTime::currentMSecsSinceEpoch();

	//float deltaT = now - _lasttime;

	//float fps = _currentFrame / deltaT * 1000;

	//setWindowTitle(QString::number(fps));
}

void drumduino::updateGraph()
{
	QVector<qreal> x(1024);
	QVector<qreal> y(1024);

	for(auto i = 0; i < 1024; ++i) {
		x[i] = i;
	}

	for(auto i = 0; i < PORT_CNT * CHAN_CNT; ++i) {
		if(_plots[i]->isVisible()) {
			for(auto k = 0; k < 1024; ++k) {
				y[k] = _frameBuffer[i][k];
			}

			_plots[i]->graph(0)->setData(x, y);

			_plots[i]->replot();
		}
	}
}




void drumduino::handleFrame(const std::array<byte, PORT_CNT* CHAN_CNT>& frame, const uint64_t currentIndex)
{

	auto fnMidiNoteOn = [this](size_t channel, byte newValue) {
		auto note = 50 + channel;
		auto velocity = newValue;

		byte data[] = {0x90 | channel, 0x7f & note , 0x7f & velocity };
		std::vector<byte> message(sizeof(data));
		memcpy(message.data(), data, message.size());

		_midiOut->send(message);
	};



	for(auto channel = 0; channel < PORT_CNT * CHAN_CNT; ++channel) {
		auto curTime = QDateTime::currentMSecsSinceEpoch();
		auto newValue = frame[channel];
		auto lastValue = _frameBuffer[mapChannels(channel)][currentIndex];

		auto& state = _states[channel];
		auto& trigger = _triggers[channel];
		auto& max = _max[channel];


		//switch(channelSettings.type) {
		//  case TypePiezo: {
		switch(state) {
			// In this state we wait for a signal to trigger
			case StateAwait: {
				if(newValue > lastValue + 35) {
					state = StateScan;
					trigger = curTime;
				}

				lastValue = newValue;
				max = newValue;

				break;
			}

			// In this state we measure the value for the given time period to get the max value
			case StateScan: {
				if(curTime < trigger + 25) {
					max = newValue > max ? newValue : max;
				}

				else {
					fnMidiNoteOn(channel, max);
					state = StateMask;
				}

				break;
			}

			// In this state we do nothing to prevent retriggering
			case StateMask: {
				if(curTime >= trigger + 25 + 30) {
					state = StateAwait;
					lastValue = newValue;
				}

				break;
			}
		}

		//  }
		//}






	}
}