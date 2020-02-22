#include "MidiTrackCombiner.h"
#include "Core/Range/Mutation/Heap.h"
#include "Core/Functional.h"

INTRA_BEGIN
bool MidiTrackCombiner::trackTimeComparer(const MidiTrackParser& a, const MidiTrackParser& b)
{
	return a.NextEventTime(mState) > b.NextEventTime(mState);
}

MidiTrackCombiner::MidiTrackCombiner(short headerTimeFormat):
	mState(headerTimeFormat) {}

void MidiTrackCombiner::AddTrack(MidiTrackParser track)
{
	if(track.Empty()) return;
	HeapContainerPush(mTracks, Move(track),
		ObjectMethod(this, &MidiTrackCombiner::trackTimeComparer));
}

void MidiTrackCombiner::ProcessEvent(IMidiDevice& device)
{
	auto& trackWithNearestEvent = HeapPop(mTracks, ObjectMethod(this, &MidiTrackCombiner::trackTimeComparer));
	const MidiTime prevTickDuration = mState.TickDuration;
	trackWithNearestEvent.ProcessEvent(mState, device);
	const MidiTime lastEventTime = trackWithNearestEvent.Time;
	if(trackWithNearestEvent.Empty()) mTracks.RemoveLast();
	else HeapPush(mTracks, ObjectMethod(this, &MidiTrackCombiner::trackTimeComparer));
	if(mState.TickDuration != prevTickDuration)
	{
		for(auto& track: mTracks) track.OnTempoChange(lastEventTime, prevTickDuration);
		HeapBuild(mTracks, ObjectMethod(this, &MidiTrackCombiner::trackTimeComparer));
	}
}

MidiTime MidiTrackCombiner::NextEventTime() const
{return mTracks.First().NextEventTime(mState);}
INTRA_END
