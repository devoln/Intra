#include "Audio/Synth/DrumInstrument.h"
#include "Audio/AudioBuffer.h"
#include "Range/Mutation/Transform.h"

namespace Intra { namespace Audio { namespace Synth {

void DrumInstrument::GetNoteSamples(Span<float> inOutSamples,
	MusicNote note, float tempo, float volume, uint sampleRate, bool add) const
{
	AudioBuffer& bufRef = cache_note(note, tempo, sampleRate);
	if(Math::Abs(volume - 1.0f) > 0.001f)
	{
		Array<float> bufSamples;
		if(add)
		{
			bufSamples = bufRef.Samples.Take(inOutSamples.Length());
			Multiply(bufSamples.AsRange(), volume);
			Add(inOutSamples.Take(bufSamples.Length()), bufSamples.AsConstRange());
		}
		else Multiply(inOutSamples, bufRef.Samples.AsConstRange().Take(inOutSamples.Length()), volume);
		return;
	}

	CSpan<float> bufSampleRange = bufRef.Samples.AsConstRange().Take(inOutSamples.Length());
	if(!add) Memory::CopyBits(inOutSamples.Take(bufSampleRange.Length()), bufSampleRange);
	else Add(inOutSamples.Take(bufSampleRange.Length()), bufSampleRange);
}


void DrumInstrument::PrepareToPlay(const MusicTrack& track, uint sampleRate) const
{
	for(size_t i=0; i<track.Notes.Count(); i++)
		cache_note(track.Notes[i].Note, track.Tempo, sampleRate);
}

AudioBuffer& DrumInstrument::cache_note(MusicNote note, float tempo, uint sampleRate) const
{
	uint id = note.Octave*12u+uint(note.Note);
	auto gen = Generators.Get(id, null);
	AudioBuffer& bufRef = SamplesCache[gen];
	if(gen==null) return bufRef;
	const size_t noteSampleCount = GetNoteSampleCount(note, tempo, sampleRate);
	if(bufRef.SampleRate!=sampleRate || noteSampleCount>bufRef.Samples.Count())
	{
		bufRef.Samples.SetCountUninitialized(noteSampleCount+bufRef.Samples.Count()/2);
		bufRef.SampleRate = sampleRate;
		const MusicNote drumNote = MusicNote(4, MusicNote::NoteType::C, ushort(note.Duration*tempo));
		gen->GetNoteSamples(bufRef.Samples, drumNote, 1, 1, sampleRate, false);
	}
	return bufRef;
}

}}}
