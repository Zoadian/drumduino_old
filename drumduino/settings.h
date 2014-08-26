#pragma once

//========================================================================================================================
//========================================================================================================================
// Settings
//========================================================================================================================
//========================================================================================================================
enum Type {
	TypeDisabled,
	TypePiezo,
};

enum Curve {
	Normal,
	Exp,
	Log,
	Sigma,
	Flat,
	eXTRA,
};

struct CurveSettings {
	Curve type;
	uint8_t value;
	int8_t offset;
	uint8_t factor;

	CurveSettings()
		: type(Normal)
		, value(127)
		, offset(0)
		, factor(127)
	{}
};

struct ChannelSettings {
	Type type;
	uint8_t note;
	uint8_t threshold;
	qint64 scanTime;
	qint64 maskTime;
	CurveSettings curve;

	ChannelSettings()
		: type(TypeDisabled)
		, note(35)
		, threshold(25)
		, scanTime(4)
		, maskTime(10)
	{}
};

struct Settings {
	uint8_t midiChannel;
	byte prescaler;
	byte throttle;
	ChannelSettings channelSettings[PORT_CNT* CHAN_CNT];

	Settings()
		: prescaler(2)
		, throttle(1)
		, midiChannel(1)
	{
	}
};