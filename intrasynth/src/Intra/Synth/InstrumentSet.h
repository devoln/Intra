#pragma once

#include "Types.h"

INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS

namespace Intra { INTRA_BEGIN

namespace Audio {

namespace Midi {
struct MidiFileInfo;
}
}
}

struct MusicalInstrument;

struct MidiInstrumentSet
{
	MusicalInstrument* Instruments[128]{nullptr};
	GenericDrumInstrument* DrumInstruments[128]{nullptr};

	void Preload(const Audio::Midi::MidiFileInfo& info, unsigned sampleRate);
};

INTRA_WARNING_POP
