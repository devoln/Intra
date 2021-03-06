#pragma once

#include "Types.h"

INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS

namespace Intra { namespace Audio {

namespace Midi {
struct MidiFileInfo;
}

namespace Synth {

struct MusicalInstrument;

struct MidiInstrumentSet
{
	MusicalInstrument* Instruments[128]{null};
	GenericDrumInstrument* DrumInstruments[128]{null};

	void Preload(const Midi::MidiFileInfo& info, uint sampleRate);
};

}

}}

INTRA_WARNING_POP
