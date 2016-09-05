#pragma once
#include "Music.h"

namespace Intra {

struct MidiDevice
{
	IMusicalInstrument* Instruments[128];
};

Music ReadMidiFile(StringView path);
Music ReadMidiFile(ArrayRange<const byte> fileData);

}
