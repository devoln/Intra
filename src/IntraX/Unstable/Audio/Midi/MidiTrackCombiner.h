#pragma once

#include "IntraX/Container/Sequential/Array.h"

#include "MidiTrackParser.h"
#include "MidiDeviceState.h"
#include "Messages.h"

INTRA_BEGIN
class MidiTrackCombiner
{
	Array<MidiTrackParser> mTracks;
	MidiDeviceState mState;
public:
	explicit MidiTrackCombiner(short headerTimeFormat);

	void AddTrack(MidiTrackParser track);

	void ProcessEvent(IMidiDevice& device);
	void ProcessAllEvents(IMidiDevice& device) {while(!mTracks.Empty()) ProcessEvent(device);}

	INTRA_FORCEINLINE bool Empty() const noexcept {return mTracks.Empty();}
	MidiTime NextEventTime() const;

private:
	bool trackTimeComparer(const MidiTrackParser& a, const MidiTrackParser& b);
};
INTRA_END
