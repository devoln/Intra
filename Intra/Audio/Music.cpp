#include "Audio/Music.h"
#include "Audio/AudioBuffer.h"
#include "Algo/Mutation/Fill.h"
#include "Audio/Sources/MusicSynthSource.h"
#include "Cpp/Warnings.h"

namespace Intra { namespace Audio {

INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS

using namespace Math;

const float MusicNote::BasicFrequencies[12] = {
	16.352f, 17.324f, 18.354f, 19.445f, 20.602f, 21.827f,
	23.125f, 24.500f, 25.957f, 27.500f, 29.135f, 30.868f
};


double MusicTrack::Duration() const
{
	double lastBeginOfNote=0, result=0;
	for(uint i=0; i<Notes.Count(); i++)
	{
		lastBeginOfNote += GetNoteTimeOffset(i);
		result = Max(result, lastBeginOfNote+Notes[i].Note.AbsDuration(Tempo));
	}
	return result;
}

MusicNote MusicTrack::operator[](size_t index) const
{
	auto result = Notes[index].Note;
	if(result.IsPause() || ToneOffset==0) return result;
	result.Note = MusicNote::NoteType(result.Note+ToneOffset%12);
	if(result.Note>=12)
	{
		result.Note = MusicNote::NoteType(result.Note-12);
		result.Octave++;
	}
	result.Octave = byte(result.Octave + ToneOffset/12);
	return result;
}

AudioBuffer MusicTrack::GetSamples(uint sampleRate) const
{
	INTRA_DEBUG_ASSERT(Instrument!=null);
	const auto duration = Duration();
	AudioBuffer result(size_t(duration*sampleRate), sampleRate);
	if(result.Samples==null) return result;
	Algo::FillZeros(result.Samples);
	uint samplePos = 0;
	for(uint i=0; i<Notes.Count(); i++)
	{
		samplePos += uint(GetNoteTimeOffset(i)*sampleRate);
		if(Notes[i].Note.IsPause()) continue;
		Instrument->GetNoteSamples(Range::Drop(result.Samples, samplePos),
			operator[](i), Tempo, Volume*Notes[i].Volume, sampleRate);
	}
	return result;
}



double Music::Duration() const
{
	double result=0;
	for(auto& track: Tracks)
		result = Max(result, track.Duration());
	return result;
}



AudioBuffer Music::GetSamples(uint sampleRate) const
{
	Sources::MusicSynthSource src(*this, sampleRate);
	AudioBuffer result(src.SampleCount());
	src.GetInterleavedSamples(result.Samples);
	return result;
}

INTRA_WARNING_POP

}}
