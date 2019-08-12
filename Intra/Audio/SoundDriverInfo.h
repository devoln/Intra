#pragma once

#include "Core/Core.h"

//TODO: audio device enumeration and selection

INTRA_BEGIN
inline namespace Audio {

struct SoundDeviceInfo
{
	static SoundDeviceInfo Get(bool* oSupported = null);

	uint SampleRate = 44100;
	ushort Channels = 2;
	ushort BitDepth = 16;
};

}
INTRA_END
