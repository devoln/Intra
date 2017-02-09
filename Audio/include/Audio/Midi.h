#pragma once

#include "Range/ForwardDecls.h"
#include "Synth/Types.h"
#include "Audio/Music.h"

namespace Intra { namespace Audio {

struct MidiDevice
{
	Synth::IMusicalInstrument* Instruments[128];
};

Music ReadMidiFile(StringView path);
Music ReadMidiFile(ArrayRange<const byte> fileData);

}}
