#pragma once



#include "MusicalInstrument.h"
#include "Types.h"
#include "RecordedSampler.h"
#include "WaveTable.h"

#include "Container/Sequential/String.h"
#include "Container/Associative/HashMap.h"

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

	GenericDrumInstrument UniDrum, AcousticBassDrum, ClosedHiHat, AcousticSnare;
};


INTRA_WARNING_POP
