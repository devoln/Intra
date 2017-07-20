#include "InstrumentSet.h"
#include "Audio/Midi/MidiFileParser.h"

INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS

namespace Intra { namespace Audio { namespace Synth {

void MidiInstrumentSet::Preload(const Midi::MidiFileInfo& info, uint sampleRate)
{
	for(size_t i = 0; i < 128; i++)
	{
		if(info.UsedDrumInstrumentsFlags[i]) (*DrumInstruments[i])(1, sampleRate);
	}
}

}}}

INTRA_WARNING_POP
