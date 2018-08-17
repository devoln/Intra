#include "DeviceState.h"

#include "Utils/Span.h"
#include "Range/Mutation/Fill.h"

INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS

namespace Intra { namespace Audio { namespace Midi {

DeviceState::DeviceState(short headerTimeFormat):
	InstrumentIds{0}, HeaderTimeFormat(headerTimeFormat)
{
	Range::Fill(Volumes, byte(127));
	Range::Fill(Pans, byte(64));
	if(headerTimeFormat < 0)
	{
		const uint framesPer100Seconds = (headerTimeFormat >> 8) == 29? 2997: 100*(headerTimeFormat >> 8);
		TickDuration = MidiTime(100) / (framesPer100Seconds * (headerTimeFormat & 0xFF));
	}
	else TickDuration = MidiTime(1) / MidiTime(2*headerTimeFormat);
}

}}}

INTRA_WARNING_POP
