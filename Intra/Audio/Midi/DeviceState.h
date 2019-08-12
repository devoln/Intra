#pragma once

#include "Core/Core.h"
#include "Math/FixedPoint.h"

INTRA_BEGIN
namespace Audio { namespace Midi {

typedef Fixed32 MidiTime;

struct DeviceState
{
	DeviceState(short headerTimeFormat):
		TickDuration(headerTimeFormat < 0?
			MidiTime(100) / (framesPer100Seconds(headerTimeFormat >> 8) * (headerTimeFormat & 0xFF)):
			MidiTime(1) / MidiTime(2*headerTimeFormat)
		),
		HeaderTimeFormat(headerTimeFormat) {}

	byte InstrumentIds[16]{};
	byte Volumes[16]{127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127};
	byte Pans[16]{64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64};
	MidiTime TickDuration{};
	short HeaderTimeFormat = 0;

private:
	static int framesPer100Seconds(int headerTimeFormatMSB)
	{
		return headerTimeFormatMSB == 29? 2997: 100*headerTimeFormatMSB;
	}
};

}}
INTRA_END
