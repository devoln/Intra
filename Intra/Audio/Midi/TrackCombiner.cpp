#include "TrackCombiner.h"
#include "Range/Mutation/Heap.h"

namespace Intra { namespace Audio { namespace Midi {

bool TrackCombiner::trackTimeComparer(const TrackParser& a, const TrackParser& b) const
{
	return a.NextEventTime(mState) > b.NextEventTime(mState);
}

TrackCombiner::TrackCombiner(short headerTimeFormat):
	mState(headerTimeFormat) {}

void TrackCombiner::AddTrack(TrackParser track)
{
	const TrackTimeComparer comparer{*this};
	mTracks.AddLast(Cpp::Move(track));
	Intra::z_D::HeapPushCore(mTracks, mTracks.Length(), comparer);
}

void TrackCombiner::ProcessEvent(IDevice& device)
{
	const TrackTimeComparer comparer{*this};
	Cpp::Swap(mTracks.First(), mTracks.Last());
	Intra::z_D::HeapOrderCore(mTracks, mTracks.Length() - 1, 0, comparer);
	auto& trackWithNearestEvent = mTracks.Last();
	const double prevTickDuration = mState.TickDuration;
	trackWithNearestEvent.ProcessEvent(mState, device);
	const double lastEventTime = trackWithNearestEvent.Time;
	if(trackWithNearestEvent.Empty()) mTracks.RemoveLast();
	else Intra::z_D::HeapPushCore(mTracks, mTracks.Length(), comparer);
	if(mState.TickDuration != prevTickDuration)
	{
		for(auto& track: mTracks) track.OnTempoChange(lastEventTime, prevTickDuration);
		Intra::z_D::HeapBuildCore(mTracks, mTracks.Length(), comparer);
	}
}

double TrackCombiner::NextEventTime() const
{return mTracks.First().NextEventTime(mState);}

}}}
