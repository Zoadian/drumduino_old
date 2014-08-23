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

	if(available < 2 + PORT_CNT * CHAN_CNT) {
		return false;
	}

	byte sentinel;
	serial->readBytes(&sentinel, 1);

	if(sentinel != 0xf0) {
		goto AGAIN;
	}

	byte manufacturer;
	serial->readBytes(&manufacturer, 1);

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

						if(currentValue < lastValue + channelSettings.thresold) {
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

						midiNoteOn(midiOut, settings.midiChannel, channelSettings.note, maxValue);
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

	ui.cbPrescaler->setCurrentIndex(_settings.prescaler);
	ui.sbThrottle->setValue(_settings.throttle);

	connect(ui.cbPrescaler, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), [this](int index) {
		_settings.prescaler = index;
		sendSysexPrescalerThrottle(_serial, _settings.prescaler, _settings.throttle);
	});

	connect(ui.sbThrottle, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged), [this](int value) {
		_settings.throttle = value;
		sendSysexPrescalerThrottle(_serial, _settings.prescaler, _settings.throttle);
	});

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
		}
	}

	// action Save
	connect(ui.actionSave, &QAction::triggered, [this]() {
		auto fileName = QFileDialog::getSaveFileName(this, "Save", 0, tr("drumduino (*.edrum)"));

		QFile file(fileName);
		file.write((const char*)&_settings, sizeof(_settings));
		file.close();
	});

	// action Load
	connect(ui.actionLoad, &QAction::triggered, [this]() {
		auto fileName = QFileDialog::getOpenFileName(this, "Open", 0, tr("drumduino (*.edrum)"));
		QFile file(fileName);
		file.read((char*)&_settings, sizeof(_settings));
		file.close();
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














#if 0

	return;



	_startTime = QDateTime::currentMSecsSinceEpoch();

	for(auto i = 0; i < 5; ++i) {
		_settings.channelSettings[0].type = TypePiezo;
	}

	for(auto i = 0; i < PORT_CNT; ++i) {
		ui.tabWidget->addTab(new PortTab(), "Port_" + QString::number(i));
		auto tab = ui.tabWidget->widget(i);
		auto table = tab->findChild<QTableWidget*>("tableWidget");
		table->setRowCount(8);
		table->setColumnCount(9);

		QStringList headers;
		headers << "type" << "note" << "thresold" << "scanTime" << "maskTime" << "CurveType" << "CurveValue" << "CurveForm" << "Graph";
		table->setHorizontalHeaderLabels(headers);
	}

	for(auto channel = 0; channel < PORT_CNT * CHAN_CNT; ++channel) {

		_settings.channelSettings[channel].note = 35 + channel;


		auto tab = ui.tabWidget->widget(channel / 8);
		auto table = tab->findChild<QTableWidget*>("tableWidget");
		table->setRowHeight(channel % 8, 110);

		_plots.push_back(new QCustomPlot(table));


		auto valueGraph = _plots.back()->addGraph();
		valueGraph->setScatterStyle(QCPScatterStyle(QCPScatterStyle::ScatterShape::ssDisc, 3));

		_plots.back()->xAxis->setRange(0, BufferSize);
		_plots.back()->yAxis->setRange(0, 256);
		_plots.back()->yAxis2->setRange(0, 2);
		_plots.back()->yAxis2->setVisible(true);

		auto stateGraph = _plots.back()->addGraph(_plots.back()->xAxis, _plots.back()->yAxis2);
		stateGraph->setPen(QPen(Qt::red));
		stateGraph->setLineStyle(QCPGraph::LineStyle::lsStepLeft);



		auto wgtType = new QComboBox(table);
		auto wgtNote = new QSpinBox(table);
		auto wgtThresold = new QSpinBox(table);
		auto wgtScanTime = new QSpinBox(table);
		auto wgtMaskTime = new QSpinBox(table);
		auto wgtCurveType = new QComboBox(table);
		auto wgtCurveValue = new QSpinBox(table);

		QStringList types;
		types << "Disabled" << "Piezo";
		wgtType->addItems(types);

		QStringList curveTypes;
		curveTypes << "Normal" << "Exp" << "Log" << "Sigma" << "Flat" << "eXTRA",
		           wgtCurveType->addItems(curveTypes);

		auto curveForm = new QCustomPlot(table);
		curveForm->addGraph();
		curveForm->xAxis->setRange(0, 127);
		curveForm->yAxis->setRange(0, 127);

		table->setCellWidget(channel % 8, 0, wgtType);
		table->setCellWidget(channel % 8, 1, wgtNote);
		table->setCellWidget(channel % 8, 2, wgtThresold);
		table->setCellWidget(channel % 8, 3, wgtScanTime);
		table->setCellWidget(channel % 8, 4, wgtMaskTime);
		table->setCellWidget(channel % 8, 5, wgtCurveType);
		table->setCellWidget(channel % 8, 6, wgtCurveValue);
		table->setCellWidget(channel % 8, 7, curveForm);
		table->setCellWidget(channel % 8, 8, _plots.back());



		wgtType->setCurrentIndex(_settings.channelSettings[channel].type);
		wgtNote->setValue(_settings.channelSettings[channel].note);
		wgtThresold->setValue(_settings.channelSettings[channel].thresold);
		wgtScanTime->setValue(_settings.channelSettings[channel].scanTime);
		wgtMaskTime->setValue(_settings.channelSettings[channel].maskTime);
		wgtCurveType->setCurrentIndex(_settings.channelSettings[channel].curveType);
		wgtCurveValue->setMaximum(256);
		wgtCurveValue->setValue(_settings.channelSettings[channel].curveValue);

		auto fnReplotCurveForm = [this, channel, curveForm]() {
			QVector<qreal> x(127);
			QVector<qreal> y(127);

			for(auto i = 0; i < 127; ++i) {
				x[i] = i;
				y[i] = calcCurve(_settings.channelSettings[channel].curveType, i, _settings.channelSettings[channel].curveValue);
			}

			curveForm->graph(0)->setData(x, y);
			curveForm->replot();
		};
		fnReplotCurveForm();


		connect(wgtType, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), [this, channel](int i) mutable { _settings.channelSettings[channel].type = (Type)i; });
		connect(wgtNote, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged), [this, channel](int i) mutable { _settings.channelSettings[channel].note = i; });
		connect(wgtThresold, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged), [this, channel](int i) mutable { _settings.channelSettings[channel].thresold = i; });
		connect(wgtScanTime, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged), [this, channel](int i) mutable { _settings.channelSettings[channel].scanTime = i; });
		connect(wgtMaskTime, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged), [this, channel](int i) mutable { _settings.channelSettings[channel].maskTime = i; });
		connect(wgtCurveType, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), [this, channel, curveForm, fnReplotCurveForm](int v) mutable {
			_settings.channelSettings[channel].curveType = (Curve)v;
			fnReplotCurveForm();
		});
		connect(wgtCurveValue, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged), [this, channel, fnReplotCurveForm](int i) mutable {
			_settings.channelSettings[channel].curveValue = i;
			fnReplotCurveForm();
		});
	}


	_serial = std::make_shared<Serial>(L"COM3", 115200);
	_midiOut = std::make_shared<MidiOut>(1);


	_workerThread = new WorkerThread(this);
	_workerThread->start();

#if 0
	{
		QTimer* timer = new QTimer(this);
		_lasttime = QDateTime::currentMSecsSinceEpoch();
		connect(timer, &QTimer::timeout, this, &drumduino::serialRead);
		timer->start(1);
	}
#endif

	{
		QTimer* timer = new QTimer(this);
		connect(timer, &QTimer::timeout, this, &drumduino::updateGraph);
		timer->start(1000 / 12);
	}

	ui.chkUpdateGraph->QCheckBox::setCheckState(_updateGraph ? Qt::Checked : Qt::Unchecked);
	connect(ui.chkUpdateGraph, &QCheckBox::stateChanged, [this](int s) {
		_updateGraph = s == Qt::Checked;
	});

#endif
}

Drumduino::~Drumduino()
{
	_drumduinoThread->stop();
	_drumduinoThread->wait();
}




#if 0

void Drumduino::serialRead()
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

	auto currentIndex = _currentFrame % BufferSize;
	handleFrame(frame, currentIndex);


	for(size_t i = 0; i < PORT_CNT * CHAN_CNT; ++i) {
		_stateBuffer[i][currentIndex] = _states[i];
		_maxVal[i] = std::max(_maxVal[i], frame[i]);
	}

	++_currentFrame;

//	goto AGAIN;
}



void Drumduino::updateGraph()
{
	if(!_updateGraph) {
		return;
	}

	auto currentIndex = _currentFrame % BufferSize;
	QVector<qreal> x(BufferSize);
	QVector<qreal> y(BufferSize);
	QVector<qreal> s(BufferSize);

	for(auto i = 0; i < BufferSize; ++i) {
		x[i] = i;
	}

	for(auto i = 0; i < PORT_CNT * CHAN_CNT; ++i) {
		_plots[i]->xAxis->setRange(x.front(), x.back());

		//if(_plots[i]->isVisible()) {
		for(auto k = 0; k < BufferSize; ++k) {
			y[k] = _frameBuffer[i][k];
			s[k] = _stateBuffer[i][k];
		}

		_plots[i]->graph(0)->setData(x, y);
		_plots[i]->graph(1)->setData(x, s);

		_plots[i]->replot();
		//}
	}
}


#endif

#if 0

void Drumduino::handleFrame(const std::array<byte, PORT_CNT* CHAN_CNT>& frame, const uint64_t currentIndex)
{

	auto fnMidiNoteOn = [this](size_t channel, byte newValue) {
		const auto& channelSettings = _settings.channelSettings[channel];

		auto note = channelSettings.note;
		auto velocity = calcCurve(channelSettings.curveType, newValue / 2, channelSettings.curveValue);

		byte data[] = {0x90 | channel, 0x7f & note , 0x7f & velocity };
		std::vector<byte> message(sizeof(data));
		memcpy(message.data(), data, message.size());

		_midiOut->send(message);
	};



	for(auto channel = 0; channel < PORT_CNT * CHAN_CNT; ++channel) {
		//const auto curTime = QDateTime::currentMSecsSinceEpoch();
		const auto curTime = _currentFrame;
		const auto newValue = frame[mapChannels(channel)];
		auto& lastValue = _frameBuffer[channel][(currentIndex - 1) % BufferSize];
		auto& nextValue = _frameBuffer[channel][currentIndex];

		auto& state = _states[channel];
		auto& triggerFrame = _triggers[channel];
		auto& maxValue = _max[channel];

		const auto& channelSettings = _settings.channelSettings[channel];

		switch(channelSettings.type) {
			case TypePiezo: {
				switch(state) {
					// In this state we wait for a signal to trigger
					case StateAwait: {
						if(newValue > lastValue + channelSettings.thresold) {
							state = StateScan;
							triggerFrame = curTime;
							maxValue = newValue;

							if(channelSettings.scanTime == 0) {
								fnMidiNoteOn(channel, maxValue);
							}
						}

						break;
					}

					// In this state we measure the value for the given time period to get the max value
					case StateScan: {
						if(curTime > triggerFrame + channelSettings.scanTime) {
							if(channelSettings.scanTime != 0) {
								fnMidiNoteOn(channel, maxValue);
							}

							state = StateMask;
						}

						else {
							maxValue = std::max(newValue, maxValue);
						}

						break;
					}

					// In this state we do nothing to prevent retriggering
					case StateMask: {
						if(curTime > triggerFrame + channelSettings.scanTime + channelSettings.maskTime) {
							state = StateAwait;
						}

						break;
					}

					default: {
						state = StateAwait;
					}
				}

				nextValue = newValue;
			}
		}
	}
}

#endif