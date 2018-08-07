#pragma once

#include "Types.h"

INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS

namespace Intra {
namespace Audio {

namespace Midi {
struct MidiFileInfo;
}
}
}

struct MusicalInstrument;

struct MidiInstrumentSet
{
	MusicalInstrument* Instruments[128]{null};
	GenericDrumInstrument* DrumInstruments[128]{null};

	void Preload(const Audio::Midi::MidiFileInfo& info, uint sampleRate);
};

INTRA_WARNING_POP
