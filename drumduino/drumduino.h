#ifndef DRUMDUINO_H
#define DRUMDUINO_H

#include <QtWidgets/QMainWindow>
#include "ui_drumduino.h"

#include "midi.h"
#include "serial.h"
#include "qcustomplot.h"

enum State {
	StateAwait,
	StateScan,
	StateMask,
};


class drumduino : public QMainWindow
{
	Q_OBJECT

public:
	drumduino(QWidget* parent = 0);
	~drumduino();

private:
	Ui::drumduinoClass ui;
	std::vector<QCustomPlot*> _plots;


	qint64 _lasttime;

private:
	std::shared_ptr<Serial> _serial;
	std::shared_ptr<MidiOut> _midiOut;

	uint64_t _currentFrame;
	std::array<std::array<byte, 1024>, PORT_CNT* CHAN_CNT> _frameBuffer;
	std::array<State, PORT_CNT* CHAN_CNT> _states;
	std::array<uint64_t, PORT_CNT* CHAN_CNT> _triggers;
	std::array<byte, PORT_CNT* CHAN_CNT> _max;
private:
	void serialRead();
	void updateGraph();
	void handleFrame(const std::array<byte, PORT_CNT* CHAN_CNT>& frame, const uint64_t currentIndex);
};

#endif // DRUMDUINO_H
