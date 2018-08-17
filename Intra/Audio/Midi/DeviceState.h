#pragma once

#include "Cpp/Warnings.h"
#include "Cpp/Features.h"
#include "Cpp/Fundamental.h"
#include "Math/FixedPoint.h"

INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS

namespace Intra { namespace Audio { namespace Midi {

typedef Fixed32 MidiTime;

struct DeviceState
{
	DeviceState(short headerTimeFormat);

	byte InstrumentIds[16];
	byte Volumes[16];
	byte Pans[16];
	MidiTime TickDuration = 0;
	short HeaderTimeFormat = 0;
};

}}}

INTRA_WARNING_POP
