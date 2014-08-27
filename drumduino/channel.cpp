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

	//QPalette::ColorRole
	ui.dialFactor->setBackgroundRole(QPalette::ColorRole::Highlight);
	//ui.dialFactor->setPalette(qApp->palette());

	QGraphicsDropShadowEffect* effect = new QGraphicsDropShadowEffect();
	effect->setBlurRadius(30);
	effect->setOffset(0, 0);
	ui.btnSumScan->setGraphicsEffect(effect);

	_curvePlot->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding);
	_curvePlot->setBackground(qApp->palette().button());
	QPen pen(qApp->palette().midlight().color());
	pen.setStyle(Qt::PenStyle::DotLine);
	_curvePlot->xAxis->grid()->setPen(pen);
	_curvePlot->yAxis->grid()->setPen(pen);
	_curvePlot->xAxis2->grid()->setPen(pen);
	_curvePlot->yAxis2->grid()->setPen(pen);
	_curvePlot->xAxis->setTicks(false);
	_curvePlot->yAxis->setTicks(false);
	_curvePlot->xAxis2->setTicks(false);
	_curvePlot->yAxis2->setTicks(false);
	ui.layoutCurvePlot->addWidget(_curvePlot);

	QStringList notes;
	notes << "C" << "C#" << "D" << "D#" << "E" << "F" << "F#" << "G" << "G#" << "A" << "A#" << "B";

	QStringList oktaves;

	for(int i = 0; i < 128; ++i) {
		oktaves << QString::number(int(i / 12) - 1) + " " + notes[i % 12] + "";
	}


	ui.cbNote->clear();
	ui.cbNote->addItems(oktaves);

	auto curve = _curvePlot->addGraph();
	curve->setPen(QPen(qApp->palette().buttonText().color()));
	_curvePlot->xAxis->setRange(0, 127);
	_curvePlot->yAxis->setRange(0, 127);
	_curvePlot->setMinimumHeight(60);

	//auto thresholdCurve = _curvePlot->addGraph(_curvePlot->xAxis, _curvePlot->yAxis);
	//thresholdCurve->setPen(QPen(Qt::red));

	_curvePlot->axisRect()->setAutoMargins(QCP::msNone);
	_curvePlot->axisRect()->setMargins(QMargins(1, 1, 1, 1));

	ui.leName->setPlaceholderText("Channel " + QString::number(channel + 1));

	update();

	connect(ui.leName, &QLineEdit::editingFinished, [this]() {
		auto name = ui.leName->text();
		memset(_channelSettings.name, 0, sizeof(_channelSettings.name));
		auto size = std::min((size_t)name.size() * sizeof(QChar), sizeof(_channelSettings.name) - 1);
		memcpy(_channelSettings.name, name.data(), size);
		update();
	});

	connect(ui.cbSensor, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), [this](int index) {_channelSettings.type = (Type)index; update(); });

	connect(ui.cbNote, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), [this](int index) {_channelSettings.note = index; update(); });

	connect(ui.dialThreshold, &QDial::valueChanged, [this](int value) {_channelSettings.threshold = value; update(); });

	connect(ui.btnSumScan, &QPushButton::toggled, [this](bool checked) {_channelSettings.sum = checked; update(); });

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
	QString name(_channelSettings.name);
	ui.leName->setText(name);

	ui.cbSensor->setCurrentIndex(_channelSettings.type);

	ui.cbNote->setCurrentIndex(_channelSettings.note);

	ui.dialThreshold->setValue(_channelSettings.threshold);

	ui.btnSumScan->setChecked(_channelSettings.sum);

	ui.dialScanTime->setValue(_channelSettings.scanTime);

	ui.dialMaskTime->setValue(_channelSettings.maskTime);

	ui.cbCurveType->setCurrentIndex(_channelSettings.curve.type);

	ui.dialValue->setValue(_channelSettings.curve.value);

	ui.dialOffset->setValue(_channelSettings.curve.offset);

	ui.dialFactor->setValue(_channelSettings.curve.factor);

	auto effect = static_cast<QGraphicsDropShadowEffect*>(ui.btnSumScan->graphicsEffect());

	if(_channelSettings.sum) {
		effect->setColor(QColor(0xff9966));
	}

	else {
		effect->setColor(QColor(0, 0, 0, 0));
	}

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
	//  y[0] = _channelSettings.threshold;
	//  y[1] = _channelSettings.threshold;

	//  _curvePlot->graph(1)->setData(x, y);
	//}

	_curvePlot->replot();
}