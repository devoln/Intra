#include "DeviceState.h"

INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS

namespace Intra { namespace Audio { namespace Midi {

DeviceState::DeviceState(short headerTimeFormat): InstrumentIds{0}
{
	Range::Fill(Volumes, 127);
	Range::Fill(Pans, 64);
}

}}}

INTRA_WARNING_POP
