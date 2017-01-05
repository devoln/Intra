#pragma once

#include "Platform/CppWarnings.h"
#include "Audio/Music.h"
#include "Containers/HashMap.h"

namespace Intra { namespace Audio {

struct AudioBuffer;

namespace Synth {

INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS

class DrumInstrument: public IMusicalInstrument
{
public:
	DrumInstrument():
		SamplesCache(), Generators() {}

	DrumInstrument(DrumInstrument&& rhs):
		SamplesCache(Meta::Move(rhs.SamplesCache)),
		Generators(Meta::Move(rhs.Generators)) {}

	DrumInstrument& operator=(DrumInstrument&& rhs)
	{
		SamplesCache = Meta::Move(rhs.SamplesCache);
		Generators = Meta::Move(rhs.Generators);
		return *this;
	}

	DrumInstrument(const DrumInstrument& rhs) = default;
	DrumInstrument& operator=(const DrumInstrument& rhs) = default;

	~DrumInstrument();

	void GetNoteSamples(ArrayRange<float> dst, MusicNote note, float tempo,
		float volume=1, uint sampleRate=44100, bool add=false) const override;

	uint GetNoteSampleCount(MusicNote note, float tempo, uint sampleRate=44100) const override
	{
		if(note.IsPause()) return 0;
		uint id = note.Octave*12u+uint(note.Note);
		auto gen = Generators.Get(id);
		if(gen==null) return 0;
		auto result = SamplesCache.Get(gen).Samples.Count();
		if(result!=0) return uint(result);
		return gen->GetNoteSampleCount(MusicNote(4, MusicNote::NoteType::C, ushort(note.Duration*tempo)), 1, sampleRate);
	}

	void PrepareToPlay(const MusicTrack& track, uint sampleRate) const override;

	mutable HashMap<IMusicalInstrument*, AudioBuffer> SamplesCache;
	HashMap<uint, IMusicalInstrument*> Generators;

private:
	//Убедиться, что в кеше присутствуют семплы для указанной ноты длины не менее указанной
	AudioBuffer& cache_note(MusicNote note, float tempo, uint sampleRate) const;
};

INTRA_WARNING_POP

}}}

