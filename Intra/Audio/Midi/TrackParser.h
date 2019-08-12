#pragma once


#include "Core/Core.h"
#include "Core/Core.h"

#include "Core/Assert.h"

#include "Core/Range/Polymorphic/InputRange.h"

#include "Messages.h"
#include "RawEvent.h"
#include "DeviceState.h"

INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS

INTRA_BEGIN
namespace Audio { namespace Midi {

struct TrackParser
{
	InputRange<RawEvent> Events;
	MidiTime Time = 0;
	uint DelayTicksPassed = 0;

	TrackParser(InputRange<RawEvent> events, MidiTime time=0);

	TrackParser(const TrackParser&) = delete;
	TrackParser& operator=(const TrackParser&) = delete;
	TrackParser(TrackParser&&) = default;
	TrackParser& operator=(TrackParser&&) = default;

	MidiTime NextEventTime(const DeviceState& state) const
	{
		INTRA_DEBUG_ASSERT(!Events.Empty());
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

	void ProcessEvent(DeviceState& state, IDevice& device);
	forceinline bool Empty() const {return Events.Empty();}

	void processSystemEvent(DeviceState& state, const RawEvent& event);
};

}}}

INTRA_WARNING_POP
