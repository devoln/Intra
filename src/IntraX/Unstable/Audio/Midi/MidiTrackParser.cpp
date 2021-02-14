#include "MidiTrackParser.h"

namespace Intra { INTRA_BEGIN
MidiTrackParser::MidiTrackParser(InputRange<MidiRawEvent> events, MidiTime time):
	Events(Move(events)), Time(time), DelayTicksPassed(0) {}

void MidiTrackParser::ProcessEvent(MidiDeviceState& state, IMidiDevice& device)
{
	auto event = Events.Next();
	unsigned delay = event.Delay();
	if(delay > DelayTicksPassed)
	{
		delay -= DelayTicksPassed;
		DelayTicksPassed = 0;
	}
	else
	{
		DelayTicksPassed -= delay;
		delay = 0;
	}
	Time += MidiTime(delay) * state.TickDuration;

	const auto type = event.MyType();

	if(type == MidiRawEvent::Type::System)
	{
		processSystemEvent(state, event);
		return;
	}

	const byte channel = event.Channel();
	const byte data0 = event.Data0();
	const byte data1 = event.Data1();
	
	if(type == MidiRawEvent::Type::ControlModeChange)
	{
		if(data0 == 7)
		{
			state.Volumes[channel] = data1;
			device.OnChannelVolumeChange({Time, channel, data1});
		}
		if(data0 == 10)
		{
			state.Pans[channel] = data1;
			device.OnChannelPanChange({Time, channel, data1});
		}
		if(data0 == 91)
		{
			device.OnChannelReverbChange({Time, channel, data1});
		}
		if(data0 >= 123 && data0 <= 127) device.OnAllNotesOff(channel);
		return;
	}
	if(type == MidiRawEvent::Type::ProgramChange)
	{
		if(data0 < 128)
		{
			state.InstrumentIds[channel] = data0;
			device.OnChannelProgramChange({Time, channel, data0});
		}
		return;
	}

	if(type == MidiRawEvent::Type::NoteOn && data1 != 0)
	{
		MidiNoteOn noteOn;
		noteOn.Time = Time;
		noteOn.NoteOctaveOrDrumId = data0;
		noteOn.Channel = channel;
		noteOn.Velocity = data1;
		noteOn.Program = channel == 9? byte(data0 + 128): state.InstrumentIds[channel];
		device.OnNoteOn(noteOn);
		return;
	}

	if(type == MidiRawEvent::Type::NoteOff ||
		(type == MidiRawEvent::Type::NoteOn && data1 == 0))
	{
		MidiNoteOff noteOff;
		noteOff.Time = Time;
		noteOff.NoteOctaveOrDrumId = data0;
		noteOff.Velocity = type == MidiRawEvent::Type::NoteOn? byte(64): data1;
		noteOff.Channel = channel;
		device.OnNoteOff(noteOff);
		return;
	}

	if(type == MidiRawEvent::Type::PitchWheelRange)
	{
		MidiPitchBend pitchBend;
		pitchBend.Time = Time;
		pitchBend.Channel = channel;
		pitchBend.Pitch = short(((event.Data1() << 7) | event.Data0()) - 8192);
		device.OnPitchBend(pitchBend);
		return;
	}
}

void MidiTrackParser::processSystemEvent(MidiDeviceState& state, const MidiRawEvent& event)
{
	if(event.Status() == 0xFF && event.Data0() == 0x51)
	{
		if(state.HeaderTimeFormat > 0)
		{
			const int value =
				(byte(event.MetaData()[0]) << 16)|
				(byte(event.MetaData()[1]) << 8)|
				byte(event.MetaData()[2]);
			state.TickDuration = MidiTime(float(value)/(1000000.0f*float(state.HeaderTimeFormat)));
		}
		return;
	}
}

} INTRA_END
