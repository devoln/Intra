#pragma once

#include "Cpp/Warnings.h"

#include "Audio/Synth/MusicalInstrument.h"
#include "Audio/Synth/Types.h"
#include "Audio/Synth/RecordedSampler.h"
#include "Audio/Synth/WaveTable.h"

#include "Container/Sequential/String.h"
#include "Container/Associative/HashMap.h"

INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS

using Intra::Audio::Synth::MusicalInstrument;
using Intra::Audio::Synth::RecordedSampler;
using Intra::Audio::Synth::GenericDrumInstrument;
using Intra::Audio::Synth::WaveTable;
using Intra::Audio::Synth::WaveTableCache;
using Intra::HashMap;
using Intra::String;

struct InstrumentLibrary
{
	InstrumentLibrary();
	~InstrumentLibrary();

	InstrumentLibrary(const InstrumentLibrary&) = delete;
	InstrumentLibrary& operator=(const InstrumentLibrary&) = delete;

	HashMap<String, WaveTableCache> Tables;

	HashMap<String, MusicalInstrument> Instruments;

	MusicalInstrument* operator[](const String& str)
	{
		auto found = Instruments.Find(str);
		if(found.Empty())
			return nullptr;
		return &found.First().Value;
	}

	GenericDrumInstrument UniDrum, AcousticBassDrum, ClosedHiHat;

	static MusicalInstrument CreateGuitar(size_t n=15, float c=128, float d=1.5,
		float e=0.75, float f=1, float freqMult=0.5f, float volume=0.6f);
};


INTRA_WARNING_POP
