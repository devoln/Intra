﻿#pragma once


#include "Core/Core.h"

INTRA_BEGIN
namespace System {

INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS

struct RamInfo
{
	uint64 TotalPhysicalMemory = 0;
	uint64 FreePhysicalMemory = 0;
	uint64 TotalVirtualMemory = 0;
	uint64 FreeVirtualMemory = 0;
	uint64 TotalSwapMemory = 0;
	uint64 FreeSwapMemory = 0;

	static RamInfo Get();
};

INTRA_WARNING_POP

}}
