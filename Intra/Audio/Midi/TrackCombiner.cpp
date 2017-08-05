#include "TrackCombiner.h"
#include "Range/Mutation/Heap.h"
#include "Funal/ObjectMethod.h"

namespace Intra { namespace Audio { namespace Midi {

bool TrackCombiner::trackTimeComparer(const TrackParser& a, const TrackParser& b)
{
	return a.NextEventTime(mState) > b.NextEventTime(mState);
}

TrackCombiner::TrackCombiner(short headerTimeFormat):
	mState(headerTimeFormat) {}

void TrackCombiner::AddTrack(TrackParser track)
{
	Range::HeapContainerPush(mTracks, Cpp::Move(track),
		ObjectMethod(this, &TrackCombiner::trackTimeComparer));
}

void TrackCombiner::ProcessEvent(IDevice& device)
{
	auto& trackWithNearestEvent = Range::HeapPop(mTracks, ObjectMethod(this, &TrackCombiner::trackTimeComparer));
	const double prevTickDuration = mState.TickDuration;
	trackWithNearestEvent.ProcessEvent(mState, device);
	const double lastEventTime = trackWithNearestEvent.Time;
	if(trackWithNearestEvent.Empty()) mTracks.RemoveLast();
	else Range::HeapPush(mTracks, ObjectMethod(this, &TrackCombiner::trackTimeComparer));
	if(mState.TickDuration != prevTickDuration)
	{
		for(auto& track: mTracks) track.OnTempoChange(lastEventTime, prevTickDuration);
		Range::HeapBuild(mTracks, ObjectMethod(this, &TrackCombiner::trackTimeComparer));
	}
}

double TrackCombiner::NextEventTime() const
{return mTracks.First().NextEventTime(mState);}

}}}
