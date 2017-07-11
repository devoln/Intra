#include "Audio/Sources/MidiSynth.h"
#include "Math/Math.h"
#include "Cpp/Warnings.h"
#include "Range/Mutation/Fill.h"

INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS

namespace Intra { namespace Audio { namespace Sources {

#ifndef INTRA_NO_MUSIC_LOADER


MidiSynth::MidiSynth(Midi::TrackCombiner music, OnCloseResourceCallback onClose, uint sampleRate, bool stereo):
	SeparateFloatAudioSource(Cpp::Move(onClose), sampleRate, ushort(stereo? 2: 1)),
	mMusic(Cpp::Move(music)),
	mTime(0),
	mSampleCount(~size_t()),
	mMinSample(1000000), mMaxSample(-1000000)
{}

size_t MidiSynth::GetUninterleavedSamples(CSpan<Span<float>> outFloatChannels)
{
	if(outFloatChannelsEmpty()) return 0;
	Span<float> dst1 = outFloatChannels.First();
	Span<float> dst2 = outFloatsChannels.Get(1);

}

#endif

}}}

INTRA_WARNING_POP
