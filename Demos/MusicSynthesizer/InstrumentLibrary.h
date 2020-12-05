#pragma once



#include "MusicalInstrument.h"
#include "Types.h"
#include "RecordedSampler.h"
#include "WaveTable.h"

#include "IntraX/Container/Sequential/String.h"
#include "IntraX/Container/Associative/HashMap.h"

INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS

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
		float e=0.75, float f=1, float freqMult=0.5f, float volume=0.6f, float ttackTime=0.005f, float releaseTime=0.2f, bool linear=false);
};


INTRA_WARNING_POP
