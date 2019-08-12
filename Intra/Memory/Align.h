﻿#pragma once

#include "Core/Core.h"
#include "Core/Assert.h"

INTRA_BEGIN
inline namespace Memory {

INTRA_NODISCARD INTRA_CONSTEXPR2 inline size_t AlignmentBytes(size_t value, size_t alignment)
{
	INTRA_DEBUG_ASSERT(alignment != 0);
	const size_t remainder = value % alignment;
	if(remainder == 0) return 0;
	return alignment - remainder;
}

INTRA_NODISCARD INTRA_CONSTEXPR2 inline size_t Aligned(size_t value, size_t alignment, size_t offset=0)
{return value + AlignmentBytes(value + offset, alignment);}

INTRA_NODISCARD inline byte* Aligned(byte* value, size_t alignment, size_t offset=0)
{return reinterpret_cast<byte*>(Aligned(reinterpret_cast<size_t>(value), alignment, offset));}

}
INTRA_END
