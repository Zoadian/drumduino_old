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
	Channel(int channel, ChannelSettings& channelSettings, QWidget *parent = 0);
	~Channel();

private:
	Ui::channel ui;

	void updateUi();
};

#endif // CHANNEL_H
