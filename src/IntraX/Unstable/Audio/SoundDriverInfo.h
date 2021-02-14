#pragma once

#include "Intra/Core.h"

//TODO: audio device enumeration and selection

namespace Intra { INTRA_BEGIN
struct SoundDeviceInfo
{
	static SoundDeviceInfo Get(bool* oSupported = nullptr);

	unsigned SampleRate = 44100;
	uint16 Channels = 2;
	uint16 BitDepth = 16;
};
} INTRA_END
