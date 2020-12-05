#include "InstrumentSet.h"
#include "IntraX/Unstable/Audio/Midi/MidiFileParser.h"

INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS

void MidiInstrumentSet::Preload(const Audio::Midi::MidiFileInfo& info, unsigned sampleRate)
{
	for(size_t i = 0; i < 128; i++)
	{
		if(info.UsedDrumInstrumentsFlags[i]) (*DrumInstruments[i])(1, sampleRate);
	}
}

INTRA_WARNING_POP
