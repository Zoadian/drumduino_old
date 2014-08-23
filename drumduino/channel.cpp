#include "stdafx.h"
#include "channel.h"

#include "curve.h"



Channel::Channel(int channel, ChannelSettings& channelSettings, QWidget* parent)
	: QWidget(parent)
	, _channel(channel)
	, _channelSettings(channelSettings)
	, _curvePlot(new QCustomPlot(this))
{
	ui.setupUi(this);
	ui.layoutCurvePlot->addWidget(_curvePlot);

	QStringList notes;
	notes << "C" << "C#" << "D" << "D#" << "E" << "F" << "F#" << "G" << "G#" << "A" << "A#" << "B";

	QStringList oktaves;

	for(int i = 0; i < 128; ++i) {
		oktaves << QString::number(int(i / 12) - 1) + " " + notes[i % 12] + "";
	}

	ui.cbNote->clear();
	ui.cbNote->addItems(oktaves);

	_curvePlot->addGraph();
	_curvePlot->xAxis->setRange(0, 127);
	_curvePlot->yAxis->setRange(0, 127);
	_curvePlot->setMinimumHeight(60);

	_curvePlot->axisRect()->setAutoMargins(QCP::msNone);
	_curvePlot->axisRect()->setMargins(QMargins(1, 1, 1, 1));

	ui.leName->setPlaceholderText("Channel " + QString::number(channel));

	updateUi();

	connect(ui.cbSensor, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), [this](int index) {_channelSettings.type = (Type)index; updateUi(); });

	connect(ui.cbNote, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), [this](int index) {_channelSettings.note = index; updateUi(); });

	connect(ui.dialThresold, &QDial::valueChanged, [this](int value) {_channelSettings.thresold = value; updateUi(); });

	connect(ui.dialScanTime, &QDial::valueChanged, [this](int value) {_channelSettings.scanTime = value; updateUi(); });

	connect(ui.dialMaskTime, &QDial::valueChanged, [this](int value) {_channelSettings.maskTime = value; updateUi(); });

	connect(ui.cbCurveType, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), [this](int index) { _channelSettings.curveType = (Curve)index; updateUi(); });

	connect(ui.dialOffset, &QDial::valueChanged, [this](int value) {_channelSettings.curveValue = value; updateUi(); });

}

Channel::~Channel()
{

}

void Channel::updateUi()
{
	ui.cbSensor->setCurrentIndex(_channelSettings.type);

	ui.cbNote->setCurrentIndex(_channelSettings.note);

	ui.labelThresold->setText(QString::number(_channelSettings.thresold));
	ui.dialThresold->setValue(_channelSettings.thresold);

	ui.labelScanTime->setText(QString::number(_channelSettings.scanTime));
	ui.dialScanTime->setValue(_channelSettings.scanTime);

	ui.labelMaskTime->setText(QString::number(_channelSettings.maskTime));
	ui.dialMaskTime->setValue(_channelSettings.maskTime);

	ui.cbCurveType->setCurrentIndex(_channelSettings.curveType);

	ui.labelOffset->setText(QString::number(_channelSettings.curveValue));
	ui.dialOffset->setValue(_channelSettings.curveValue);

	QVector<qreal> x(127);
	QVector<qreal> y(127);

	for(auto i = 0; i < 127; ++i) {
		x[i] = i;
		y[i] = calcCurve(_channelSettings.curveType, i, _channelSettings.curveValue);
	}

	_curvePlot->graph(0)->setData(x, y);
	_curvePlot->replot();
}