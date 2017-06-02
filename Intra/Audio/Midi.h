#pragma once

#include "Utils/ErrorStatus.h"

#include "Range/ForwardDecls.h"

#include "Audio/Music.h"
#include "Synth/Types.h"

namespace Intra { namespace Audio {

struct MidiDevice
{
	Synth::IMusicalInstrument* Instruments[128];
};

Music ReadMidiFile(StringView path, ErrorStatus& status);
Music ReadMidiFile(CSpan<byte> fileData, ErrorStatus& status);

}}
