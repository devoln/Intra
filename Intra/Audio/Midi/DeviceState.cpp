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
		const float framesPerSecond = (headerTimeFormat >> 8) == 29? 29.97f: float(headerTimeFormat >> 8);
		TickDuration = 1.0f/framesPerSecond/float(headerTimeFormat & 0xFF);
	}
	else TickDuration = 60.0f/120/float(headerTimeFormat);
}

}}}

INTRA_WARNING_POP
