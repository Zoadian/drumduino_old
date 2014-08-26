#include "stdafx.h"
#include "drumduino.h"

#include "porttab.h"

#include "channel.h"
#include "curve.h"


size_t mapChannels(size_t channel)
{
	size_t port = channel / CHAN_CNT;
	size_t chan = channel % CHAN_CNT;

	const size_t pinMapping[8] = {2, 4, 1, 6, 0, 7, 3, 5};
	return port * CHAN_CNT + pinMapping[chan];
}

bool readNextFrame(std::shared_ptr<Serial>& serial, DrumduinoProc& proc)
{
AGAIN:
	auto available = serial->available();

	if(available < sizeof(byte) + sizeof(byte) + sizeof(unsigned long) + PORT_CNT * CHAN_CNT) {
		return false;
	}

	byte sentinel;
	serial->readBytes(&sentinel, 1);

	if(sentinel != 0xf0) {
		goto AGAIN;
	}

	byte manufacturer;
	serial->readBytes(&manufacturer, 1);

	unsigned long time;
	serial->readBytes((byte*)&time, sizeof(unsigned long));

	auto& frame = proc.frameBuffer[proc.frameCounter % BufferSize];
	serial->readBytes(frame.data(), frame.size());

	return true;
}

void sendSysexPrescalerThrottle(std::shared_ptr<Serial>& serial, byte prescaler, byte throttle)
{
	byte msg[] = {0xf0, 42, prescaler, throttle, 0xF7};
	serial->write(msg, sizeof(msg));
}

void midiNoteOn(std::shared_ptr<MidiOut>& midiOut, byte channel, byte note, byte velocity)
{
	byte data[] = {0x90 | channel, 0x7f & note , 0x7f & velocity };
	std::vector<byte> message(sizeof(data));
	memcpy(message.data(), data, message.size());
	midiOut->send(message);
}

void processFrame(std::shared_ptr<MidiOut>& midiOut, DrumduinoProc& proc, const Settings& settings)
{
	const auto& lastFrame = proc.frameBuffer[(proc.frameCounter - 1) % BufferSize];
	const auto& currentFrame = proc.frameBuffer[proc.frameCounter % BufferSize];

	for(auto channel = 0; channel < PORT_CNT * CHAN_CNT; ++channel) {
		const auto& lastValue = lastFrame[mapChannels(channel)];
		const auto& currentValue = currentFrame[mapChannels(channel)];

		auto& state = proc.states[channel];
		auto& triggerFrame = proc.triggers[channel];
		auto& maxValue = proc.maxs[channel];

		const auto& channelSettings = settings.channelSettings[channel];

		switch(channelSettings.type) {
			case TypePiezo: {
				switch(state) {
					// In this state we wait for a signal to trigger
					case StateAwait: {
STATE_AGAIN:

						if(currentValue < lastValue + channelSettings.threshold) {
							break;
						}

						state = StateScan;
						triggerFrame = proc.frameCounter;
						maxValue = currentValue;
						//fallthrough
					}

					// In this state we measure the value for the given time period to get the max value
					case StateScan: {
						if(proc.frameCounter < triggerFrame + channelSettings.scanTime) {
							maxValue = std::max(currentValue, maxValue);
							break;
						}

						midiNoteOn(midiOut, settings.midiChannel, channelSettings.note, calcCurve(channelSettings.curve, maxValue));
						state = StateMask;
						//fallthrough

					}

					// In this state we do nothing to prevent retriggering
					case StateMask: {
						if(proc.frameCounter < triggerFrame + channelSettings.scanTime + channelSettings.maskTime) {
							break;
						}

						state = StateAwait;
						goto STATE_AGAIN;
					}

					default: {
						throw std::exception("not a valid state!");
					}
				}
			}
		}
	}
}



Drumduino::Drumduino(QWidget* parent)
	: QMainWindow(parent)
{
	ui.setupUi(this);

	// Setup Channel Settings
	for(auto channel = 0; channel < PORT_CNT * CHAN_CNT; ++channel) {
		_settings.channelSettings[channel].note = channel;
	}

	// Setup Channels
	for(auto port = 0; port < PORT_CNT; ++port) {
		auto wgtPort = ui.tabWidget->widget(ui.tabWidget->addTab(new PortTab(), "Port " + QString::number(port)));

		for(auto pin = 0; pin < CHAN_CNT; ++pin) {
			auto channel = port * CHAN_CNT + pin;
			auto wgtChannel = new Channel(channel, _settings.channelSettings[channel], wgtPort);
			wgtPort->layout()->addWidget(wgtChannel);
			_channels[channel] = wgtChannel;

			for(auto dial : wgtChannel->findChildren<QDial*>()) {
				dial->installEventFilter(this);
			}

		}
	}

	// action Save
	connect(ui.actionSave, &QAction::triggered, [this]() {
		auto fileName = QFileDialog::getSaveFileName(this, "Save", 0, tr("drumduino (*.edrum)"));

		QFile file(fileName);
		file.open(QIODevice::WriteOnly);
		file.write((const char*)&_settings, sizeof(_settings));
		file.close();
	});

	// action Load
	connect(ui.actionLoad, &QAction::triggered, [this]() {
		auto fileName = QFileDialog::getOpenFileName(this, "Open", 0, tr("drumduino (*.edrum)"));
		QFile file(fileName);
		file.open(QIODevice::ReadOnly);
		file.read((char*)&_settings, sizeof(_settings));
		file.close();

		for(auto& channel : _channels) {
			channel->update();
		}
	});



	_serial = std::make_shared<Serial>(L"COM3", 115200);
	_midiOut = std::make_shared<MidiOut>(1);


	_drumduinoThread = new DrumduinoThread(this, [this]() {
		if(readNextFrame(_serial, _proc)) {
			processFrame(_midiOut, _proc, _settings);

#if 1
			_proc.stateBuffer[_proc.frameCounter % BufferSize] = _proc.states;
#endif

			++_proc.frameCounter;
		}
	});
	_drumduinoThread->start();



#if 1
	{
		std::array<QCustomPlot*, PORT_CNT* CHAN_CNT> plots;

		for(auto port = 0; port < PORT_CNT; ++port) {
			auto wgtPort = ui.tabWidget->widget(ui.tabWidget->addTab(new PortTab(), "Graph_Port " + QString::number(port)));

			auto table = new QTableWidget(CHAN_CNT, 1, wgtPort);
			table->horizontalHeader()->setStretchLastSection(true);
			table->verticalHeader()->setMinimumHeight(100);
			table->horizontalHeader()->setVisible(false);
			wgtPort->layout()->addWidget(table);

			for(auto pin = 0; pin < CHAN_CNT; ++pin) {
				auto channel = port * CHAN_CNT + pin;
				auto wgtPlot = new QCustomPlot(table);
				wgtPlot->addGraph();
				table->setRowHeight(pin, 127);
				table->setCellWidget(pin, 0, wgtPlot);

				wgtPlot->xAxis->setRange(0, BufferSize);
				wgtPlot->yAxis->setRange(0, 127);
				wgtPlot->yAxis2->setRange(0, 2);
				wgtPlot->yAxis2->setVisible(true);

				auto stateGraph = wgtPlot->addGraph(wgtPlot->xAxis, wgtPlot->yAxis2);
				stateGraph->setPen(QPen(Qt::red));
				stateGraph->setLineStyle(QCPGraph::LineStyle::lsStepLeft);

				plots[port * CHAN_CNT + pin] = wgtPlot;
			}
		}

		QTimer* timer = new QTimer(this);
		connect(timer, &QTimer::timeout, [this, plots]() {
			auto currentIndex = _proc.frameCounter % BufferSize;
			QVector<qreal> x(BufferSize);
			QVector<qreal> y(BufferSize);
			QVector<qreal> s(BufferSize);

			for(auto i = 0; i < BufferSize; ++i) {
				x[i] = i;
			}

			for(auto i = 0; i < PORT_CNT * CHAN_CNT; ++i) {
				if(plots[i]->isVisible()) {
					auto channel = mapChannels(i);

					plots[i]->yAxis->setLabel(QString(_settings.channelSettings[i].name));
					plots[i]->xAxis->setRange(x.front(), x.back());

					for(auto k = 0; k < BufferSize; ++k) {
						y[k] = _proc.frameBuffer[k][channel];
						s[k] = _proc.stateBuffer[k][i];
					}

					plots[i]->graph(0)->setData(x, y);
					plots[i]->graph(1)->setData(x, s);

					plots[i]->replot();
				}
			}
		});
		timer->start(1000 / 12);
	}
#endif
}

Drumduino::~Drumduino()
{
	_drumduinoThread->stop();
	_drumduinoThread->wait();
}