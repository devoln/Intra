#include "TrackParser.h"

INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS

INTRA_BEGIN
namespace Audio { namespace Midi {

TrackParser::TrackParser(InputRange<RawEvent> events, MidiTime time):
	Events(Move(events)), Time(time), DelayTicksPassed(0) {}

void TrackParser::ProcessEvent(DeviceState& state, IDevice& device)
{
	auto event = Events.Next();
	uint delay = event.Delay();
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

	if(type == RawEvent::Type::System)
	{
		processSystemEvent(state, event);
		return;
	}

	const byte channel = event.Channel();
	const byte data0 = event.Data0();
	const byte data1 = event.Data1();
	
	if(type == RawEvent::Type::ControlModeChange)
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
	if(type == RawEvent::Type::ProgramChange)
	{
		if(data0 < 128)
		{
			state.InstrumentIds[channel] = data0;
			device.OnChannelProgramChange({Time, channel, data0});
		}
		return;
	}

	if(type == RawEvent::Type::NoteOn && data1 != 0)
	{
		NoteOn noteOn;
		noteOn.Time = Time;
		noteOn.NoteOctaveOrDrumId = data0;
		noteOn.Channel = channel;
		noteOn.Velocity = data1;
		noteOn.Program = channel == 9? byte(data0 + 128): state.InstrumentIds[channel];
		device.OnNoteOn(noteOn);
		return;
	}

	if(type == RawEvent::Type::NoteOff ||
		(type == RawEvent::Type::NoteOn && data1 == 0))
	{
		NoteOff noteOff;
		noteOff.Time = Time;
		noteOff.NoteOctaveOrDrumId = data0;
		noteOff.Velocity = type == RawEvent::Type::NoteOn? byte(64): data1;
		noteOff.Channel = channel;
		device.OnNoteOff(noteOff);
		return;
	}

	if(type == RawEvent::Type::PitchWheelRange)
	{
		PitchBend noteBend;
		noteBend.Time = Time;
		noteBend.Channel = channel;
		noteBend.Pitch = short(((event.Data1() << 7) | event.Data0()) - 8192);
		device.OnPitchBend(noteBend);
		return;
	}
}

void TrackParser::processSystemEvent(DeviceState& state, const RawEvent& event)
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

}}}

INTRA_WARNING_POP

