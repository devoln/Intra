#pragma once

#include "Intra/Assert.h"
#include "Intra/Range/Polymorphic/InputRange.h"

#include "Messages.h"
#include "MidiRawEvent.h"
#include "MidiDeviceState.h"

INTRA_BEGIN
struct MidiTrackParser
{
	InputRange<MidiRawEvent> Events;
	MidiTime Time = 0;
	unsigned DelayTicksPassed = 0;

	MidiTrackParser(InputRange<MidiRawEvent> events, MidiTime time=0);

	MidiTrackParser(const MidiTrackParser&) = delete;
	MidiTrackParser& operator=(const MidiTrackParser&) = delete;
	MidiTrackParser(MidiTrackParser&&) = default;
	MidiTrackParser& operator=(MidiTrackParser&&) = default;

	MidiTime NextEventTime(const MidiDeviceState& state) const
	{
		INTRA_PRECONDITION(!Events.Empty());
		unsigned delay = Events.First().Delay();
		if(delay > DelayTicksPassed) delay -= DelayTicksPassed;
		else delay = 0;
		return Time + MidiTime(delay) * state.TickDuration;
	}

	void OnTempoChange(MidiTime time, MidiTime prevTickDuration)
	{
		const unsigned ticksPassed = unsigned((time - Time) / prevTickDuration + MidiTime(0.5));
		DelayTicksPassed += ticksPassed;
		Time += MidiTime(ticksPassed) * prevTickDuration;
	}

	void ProcessEvent(MidiDeviceState& state, IMidiDevice& device);
	INTRA_FORCEINLINE bool Empty() const {return Events.Empty();}

	void processSystemEvent(MidiDeviceState& state, const MidiRawEvent& event);
};

INTRA_END
