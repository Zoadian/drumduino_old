#ifndef DRUMDUINO_H
#define DRUMDUINO_H

#include <QtWidgets/QMainWindow>
#include "ui_drumduino.h"

#include "midi.h"
#include "serial.h"
#include "qcustomplot.h"
#include "settings.h"

enum { BufferSize = 512 };

enum State {
	StateAwait,
	StateScan,
	StateMask,
};

class DrumduinoThread;
class Channel;

struct DrumduinoProc {
	uint64_t frameCounter;
	std::array<std::array<byte, PORT_CNT* CHAN_CNT>, BufferSize>  frameBuffer;
	std::array<State, PORT_CNT* CHAN_CNT> states;
	std::array<uint64_t, PORT_CNT* CHAN_CNT> triggers;
	std::array<byte, PORT_CNT* CHAN_CNT> maxs;

	std::array<std::array<State, PORT_CNT* CHAN_CNT>, BufferSize>  stateBuffer;

	DrumduinoProc()
		: frameCounter(0)
	{
		for(auto& fb : frameBuffer) {
			fb.fill(0);
		}

		states.fill(StateAwait);
		triggers.fill(0);
		maxs.fill(0);

		for(auto& sb : stateBuffer) {
			sb.fill(StateAwait);
		}
	}
};

class Drumduino : public QMainWindow
{
	Q_OBJECT

public:
	Drumduino(QWidget* parent = 0);
	~Drumduino();

private:
	Ui::drumduinoClass ui;
	std::array<Channel*, PORT_CNT* CHAN_CNT> _channels;

	std::shared_ptr<Serial> _serial;
	std::shared_ptr<MidiOut> _midiOut;

	DrumduinoThread* _drumduinoThread;

	Settings _settings;
	DrumduinoProc _proc;

private:
	bool readFrame(std::array<byte, PORT_CNT* CHAN_CNT>& frame);

private:

	bool eventFilter(QObject* o, QEvent* e)
	{
		auto dial = qobject_cast<QDial*>(o);

		if(dial && e->type() == QEvent::Paint) {
			QPaintEvent* paintEvent = static_cast<QPaintEvent*>(e);


			QStylePainter p(dial);

			QStyleOptionSlider option;

			option.initFrom(dial);
			option.minimum = dial->minimum();
			option.maximum = dial->maximum();
			option.sliderPosition = dial->value();
			option.sliderValue = dial->value();
			option.singleStep = dial->singleStep();
			option.pageStep = dial->pageStep();
			option.upsideDown = !dial->invertedAppearance();
			option.notchTarget = 0;
			option.dialWrapping = dial->wrapping();
			option.subControls = QStyle::SC_All;
			option.activeSubControls = QStyle::SC_None;

			//option.subControls &= ~QStyle::SC_DialTickmarks;
			//option.tickPosition = QSlider::TicksAbove;
			option.tickPosition = QSlider::NoTicks;

			option.tickInterval = dial->notchSize();

			p.drawComplexControl(QStyle::CC_Dial, option);
			p.drawText(dial->rect(), Qt::AlignCenter, QString::number(dial->value()));
			return true;
		}

		return QMainWindow::eventFilter(o, e);
	}


#if 0

public:
	std::vector<QCustomPlot*> _plots;

	bool _updateGraph;
	qint64 _lasttime;
	qint64 _startTime;

private:

	uint64_t _currentFrame;
	std::array<std::array<byte, BufferSize>, PORT_CNT* CHAN_CNT> _frameBuffer;
	std::array<std::array<State, BufferSize>, PORT_CNT* CHAN_CNT> _stateBuffer;
	std::array<State, PORT_CNT* CHAN_CNT> _states;
	std::array<uint64_t, PORT_CNT* CHAN_CNT> _triggers;
	std::array<byte, PORT_CNT* CHAN_CNT> _max;


	std::array<byte, PORT_CNT* CHAN_CNT> _maxVal;

public:



	void serialRead();
	void updateGraph();
	void handleFrame(const std::array<byte, PORT_CNT* CHAN_CNT>& frame, const uint64_t currentIndex);
#endif
};


class DrumduinoThread : public QThread
{
	Q_OBJECT

private:
	Drumduino* _drumduino;
	bool _run;
	std::function<void()> _fnCall;

public:
	DrumduinoThread(Drumduino* drumduino, std::function<void()> fnCall)
		: QThread(drumduino)
		, _drumduino(drumduino)
		, _run(true)
		, _fnCall(fnCall)
	{}

	void run() Q_DECL_OVERRIDE {
		for(; _run;)
		{
			_fnCall();
		}
	}

	void stop()
	{
		_run = false;
	}
};

#endif // DRUMDUINO_H
