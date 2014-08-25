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

	_curvePlot->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding);
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

	//auto thresoldCurve = _curvePlot->addGraph(_curvePlot->xAxis, _curvePlot->yAxis);
	//thresoldCurve->setPen(QPen(Qt::red));

	_curvePlot->axisRect()->setAutoMargins(QCP::msNone);
	_curvePlot->axisRect()->setMargins(QMargins(1, 1, 1, 1));

	ui.leName->setPlaceholderText("Channel " + QString::number(channel + 1));

	update();

	connect(ui.cbSensor, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), [this](int index) {_channelSettings.type = (Type)index; update(); });

	connect(ui.cbNote, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), [this](int index) {_channelSettings.note = index; update(); });

	connect(ui.dialThresold, &QDial::valueChanged, [this](int value) {_channelSettings.thresold = value; update(); });

	connect(ui.dialScanTime, &QDial::valueChanged, [this](int value) {_channelSettings.scanTime = value; update(); });

	connect(ui.dialMaskTime, &QDial::valueChanged, [this](int value) {_channelSettings.maskTime = value; update(); });

	connect(ui.cbCurveType, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), [this](int index) { _channelSettings.curve.type = (Curve)index; update(); });

	connect(ui.dialValue, &QDial::valueChanged, [this](int value) {_channelSettings.curve.value = value; update(); });

	connect(ui.dialOffset, &QDial::valueChanged, [this](int value) {_channelSettings.curve.offset = value; update(); });

	connect(ui.dialFactor, &QDial::valueChanged, [this](int value) {_channelSettings.curve.factor = value; update(); });

}

Channel::~Channel()
{

}

void Channel::update()
{
	ui.cbSensor->setCurrentIndex(_channelSettings.type);

	ui.cbNote->setCurrentIndex(_channelSettings.note);

	//ui.labelThresold->setText(QString::number(_channelSettings.thresold));
	ui.dialThresold->setValue(_channelSettings.thresold);

	//ui.labelScanTime->setText(QString::number(_channelSettings.scanTime));
	ui.dialScanTime->setValue(_channelSettings.scanTime);

	//ui.labelMaskTime->setText(QString::number(_channelSettings.maskTime));
	ui.dialMaskTime->setValue(_channelSettings.maskTime);

	ui.cbCurveType->setCurrentIndex(_channelSettings.curve.type);

	//ui.labelValue->setText(QString::number(_channelSettings.curveValue));
	ui.dialValue->setValue(_channelSettings.curve.value);

	//ui.labelOffset->setText(QString::number(_channelSettings.curveOffset));
	ui.dialOffset->setValue(_channelSettings.curve.offset);

	//ui.labelFactor->setText(QString::number(_channelSettings.curveFactor));
	ui.dialFactor->setValue(_channelSettings.curve.factor);

	{
		QVector<qreal> x(128);
		QVector<qreal> y(128);

		for(auto i = 0; i < 128; ++i) {
			x[i] = i;
			y[i] = calcCurve(_channelSettings.curve, i) ;
		}

		_curvePlot->graph(0)->setData(x, y);
	}

	//{
	//  QVector<qreal> x(2);
	//  QVector<qreal> y(2);
	//
	//  x[0] = 0;
	//  x[1] = 127;
	//  y[0] = _channelSettings.thresold;
	//  y[1] = _channelSettings.thresold;

	//  _curvePlot->graph(1)->setData(x, y);
	//}

	_curvePlot->replot();
}