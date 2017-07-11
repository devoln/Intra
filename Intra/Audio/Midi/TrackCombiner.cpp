#include "TrackCombiner.h"
#include "Range/Mutation/Heap.h"
#include "Funal/ObjectMethod.h"

namespace Intra { namespace Audio { namespace Midi {

bool TrackCombiner::trackTimeComparer(const TrackParser& a, const TrackParser& b)
{
	return a.NextEventTime(mState) > b.NextEventTime(mState);
}

TrackCombiner::TrackCombiner(short headerTimeFormat, IDevice* device):
	mState(headerTimeFormat), mDevice(device) {}

void TrackCombiner::AddTrack(TrackParser track)
{
	Range::HeapContainerPush(mTracks, Cpp::Move(track),
		ObjectMethod(this, &TrackCombiner::trackTimeComparer));
}

void TrackCombiner::ProcessEvent()
{
	INTRA_DEBUG_ASSERT(mDevice != null);
	auto& track = Range::HeapPop(mTracks, ObjectMethod(this, &TrackCombiner::trackTimeComparer));
	track.ProcessEvent(mState, *mDevice);
	if(track.Empty()) mTracks.RemoveLast();
	else Range::HeapPush(mTracks, ObjectMethod(this, &TrackCombiner::trackTimeComparer));
}

double TrackCombiner::NextEventTime() const
{return mTracks.First().NextEventTime(mState);}

}}}
