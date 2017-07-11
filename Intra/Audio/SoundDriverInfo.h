#pragma once

#include "Cpp/Fundamental.h"

namespace Intra { namespace Audio {

struct SoundDeviceInfo
{
	static SoundDeviceInfo Get(bool* oSupported = null);

	uint SampleRate = 44100;
	ushort Channels = 2;
	ushort BitDepth = 16;
};

}}
