#pragma once

#pragma comment(lib, "Winmm.lib")

//========================================================================================================================
//========================================================================================================================
// Midi
//========================================================================================================================
//========================================================================================================================
class Midi
{
public:
	Midi(void);
	~Midi(void);

};


//========================================================================================================================
//========================================================================================================================
// MidiIn
//========================================================================================================================
//========================================================================================================================
class MidiIn : public Midi
{
};


//========================================================================================================================
//========================================================================================================================
// MidiOut
//========================================================================================================================
//========================================================================================================================
class MidiOut : public Midi
{
public:
	static std::vector<std::wstring> list()
	{
		std::vector<std::wstring> midiOutPortNames;
		auto midiOutPortCnt = ::midiOutGetNumDevs();

		for(unsigned int port = 0; port < midiOutPortCnt; ++port) {
			MIDIOUTCAPS deviceCaps;
			::midiOutGetDevCaps(port, &deviceCaps, sizeof(MIDIOUTCAPS));

			midiOutPortNames.push_back(deviceCaps.szPname);
		}

		return midiOutPortNames;
	}

private:
	HMIDIOUT _handle;

public:
	MidiOut(unsigned int portNumber)
	{
		if(::midiOutOpen(&_handle, portNumber, (DWORD)NULL, (DWORD)NULL, CALLBACK_NULL) != MMSYSERR_NOERROR) {
			throw std::exception("error opening midi out port");
		}
	}

	~MidiOut()
	{
		::midiOutReset(_handle);
		::midiOutClose(_handle);
	}

	void send(const std::vector<byte>& message)
	{
		if(message.empty()) {
			return;
		}

		if(message[0] == 0xF0) {

			MIDIHDR sysex;
			sysex.lpData = (LPSTR)message.data();
			sysex.dwBufferLength = message.size();
			sysex.dwFlags = 0;

			if(midiOutPrepareHeader(_handle,  &sysex, sizeof(MIDIHDR)) != MMSYSERR_NOERROR) {
				throw std::exception("error midiOutPrepareHeader");
			}

			if(midiOutLongMsg(_handle, &sysex, sizeof(MIDIHDR)) != MMSYSERR_NOERROR) {
				throw std::exception("error midiOutLongMsg");
			}

			while(MIDIERR_STILLPLAYING == midiOutUnprepareHeader(_handle, &sysex, sizeof(MIDIHDR))) {
				Sleep(1);
			}
		}

		else {
			if(message.size() > 3) {
				throw std::exception("invalid message. not a sysex and too big for normal");
			}

			//DWORD packet = MAKELONG(MAKEWORD(message[0], message[1]), MAKEWORD(message[2], 0));

			if(midiOutShortMsg(_handle, *reinterpret_cast<const DWORD*>(message.data())) != MMSYSERR_NOERROR) {
				throw std::exception("error midiOutShortMsg");
			}
		}
	}
};