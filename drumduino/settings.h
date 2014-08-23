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

struct ChannelSettings {
	Type type;
	uint8_t note;
	uint8_t thresold;
	qint64 scanTime;
	qint64 maskTime;
	Curve curveType;
	int curveValue;

	ChannelSettings()
		: type(TypeDisabled)
		, note(35)
		, thresold(70)
		, scanTime(2)
		, maskTime(5)
		, curveType(Normal)
		, curveValue(127)
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