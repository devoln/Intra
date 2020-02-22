#pragma once

#include "Core/Assert.h"
#include "Core/Range/Polymorphic/InputRange.h"

#include "Messages.h"
#include "MidiRawEvent.h"
#include "MidiDeviceState.h"

INTRA_BEGIN
struct MidiTrackParser
{
	InputRange<MidiRawEvent> Events;
	MidiTime Time = 0;
	uint DelayTicksPassed = 0;

	MidiTrackParser(InputRange<MidiRawEvent> events, MidiTime time=0);

	MidiTrackParser(const MidiTrackParser&) = delete;
	MidiTrackParser& operator=(const MidiTrackParser&) = delete;
	MidiTrackParser(MidiTrackParser&&) = default;
	MidiTrackParser& operator=(MidiTrackParser&&) = default;

	MidiTime NextEventTime(const MidiDeviceState& state) const
	{
		INTRA_PRECONDITION(!Events.Empty());
		uint delay = Events.First().Delay();
		if(delay > DelayTicksPassed) delay -= DelayTicksPassed;
		else delay = 0;
		return Time + MidiTime(delay) * state.TickDuration;
	}

	void OnTempoChange(MidiTime time, MidiTime prevTickDuration)
	{
		const uint ticksPassed = uint((time - Time) / prevTickDuration + MidiTime(0.5));
		DelayTicksPassed += ticksPassed;
		Time += MidiTime(ticksPassed) * prevTickDuration;
	}

	void ProcessEvent(MidiDeviceState& state, IMidiDevice& device);
	forceinline bool Empty() const {return Events.Empty();}

	void processSystemEvent(MidiDeviceState& state, const MidiRawEvent& event);
};

INTRA_END
