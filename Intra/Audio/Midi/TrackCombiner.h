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
public:
	explicit TrackCombiner(short headerTimeFormat);

	void AddTrack(TrackParser track);

	void ProcessEvent(IDevice& device);
	void ProcessAllEvents(IDevice& device) {while(!mTracks.Empty()) ProcessEvent(device);}

	forceinline bool Empty() const noexcept {return mTracks.Empty();}
	double NextEventTime() const;

private:
	bool trackTimeComparer(const TrackParser& a, const TrackParser& b) const;
	struct TrackTimeComparer
	{
		const TrackCombiner& Owner;
		forceinline bool operator()(const TrackParser& a, const TrackParser& b) const
		{return Owner.trackTimeComparer(a, b);}
	};
};

}}}

INTRA_WARNING_POP
