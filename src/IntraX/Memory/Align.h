#pragma once

#include "Intra/Assert.h"

INTRA_BEGIN

[[nodiscard]] constexpr inline size_t AlignmentBytes(size_t value, size_t alignment)
{
	INTRA_PRECONDITION(alignment != 0);
	const size_t remainder = value % alignment;
	if(remainder == 0) return 0;
	return alignment - remainder;
}

[[nodiscard]] constexpr inline size_t Aligned(size_t value, size_t alignment, size_t offset=0)
{return value + AlignmentBytes(value + offset, alignment);}

[[nodiscard]] inline byte* Aligned(byte* value, size_t alignment, size_t offset=0)
{return reinterpret_cast<byte*>(Aligned(reinterpret_cast<size_t>(value), alignment, offset));}

INTRA_END
