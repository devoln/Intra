#pragma once

#include "Cpp/Warnings.h"

#include "Container/Sequential/Array.h"

#include "TrackParser.h"
#include "DeviceState.h"
#include "Messages.h"

INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS

namespace Intra { namespace Audio { namespace Midi {

class TrackCombiner
{
	Array<TrackParser> mTracks;
	DeviceState mState;
	IDevice* mDevice;
public:

	explicit TrackCombiner(short headerTimeFormat, IDevice* device);

	void AddTrack(TrackParser track);

	void ProcessEvent();
	void ProcessAllEvents() {while(!mTracks.Empty()) ProcessEvent();}

	forceinline bool Empty() const noexcept {return mTracks.Empty();}
	double NextEventTime() const;

private:
	bool trackTimeComparer(const TrackParser& a, const TrackParser& b);
};

}}}

INTRA_WARNING_POP
