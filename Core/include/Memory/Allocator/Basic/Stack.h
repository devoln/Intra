#pragma once

#include "Platform/FundamentalTypes.h"
#include "Platform/CppWarnings.h"
#include "Platform/Debug.h"
#include "Range/Generators/ArrayRange.h"

namespace Intra { namespace Memory {

INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS

struct AStack
{
	AStack(ArrayRange<byte> buf, size_t allocatorAlignment):
		mStart(buf.Begin), mRest(buf), mAlignment(allocatorAlignment) {}
	
	size_t GetAlignment() const {return mAlignment;}
	AnyPtr Allocate(size_t size, SourceInfo sourceInfo);
	void Free(void* ptr, size_t size);

private:
	byte* mStart;
	ArrayRange<byte> mRest;
	size_t mAlignment;
};

}}

INTRA_WARNING_POP

