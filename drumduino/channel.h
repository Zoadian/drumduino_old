#ifndef CHANNEL_H
#define CHANNEL_H

#include <QWidget>
#include "ui_channel.h"

#include "settings.h"
#include "qcustomplot.h"

class Channel : public QWidget
{
	Q_OBJECT
private:
	int _channel;
	ChannelSettings& _channelSettings;
	QCustomPlot* _curvePlot;

public:
	Channel(int channel, ChannelSettings& channelSettings, QWidget* parent = 0);
	~Channel();

private:
	Ui::channel ui;

public:
	void update();

	void triggered(byte maxValue, byte sumValue, byte calcValue)
	{
		ui.pbMax->setValue(maxValue);
		ui.pbSum->setValue(sumValue);
		ui.pbOut->setValue(calcValue);
	}
};

#endif // CHANNEL_H
