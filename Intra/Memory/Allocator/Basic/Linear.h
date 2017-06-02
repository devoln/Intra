#pragma once

#include "Cpp/Fundamental.h"
#include "Cpp/Warnings.h"
#include "Utils/Debug.h"
#include "Utils/Span.h"

INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS

namespace Intra { namespace Memory {

struct ALinear
{
	ALinear(Span<byte> buf=null, size_t allocatorAlignment=16):
		mStart(buf.Begin), mRest(buf), mAlignment(allocatorAlignment) {}

	size_t GetAlignment() const {return mAlignment;}

	AnyPtr Allocate(size_t bytes, const Utils::SourceInfo& sourceInfo)
	{
		(void)sourceInfo;

		byte* userPtr = Aligned(mRest.Begin, mAlignment);
		if(mRest.Begin + bytes >= mRest.End) return null;
		mRest.Begin += bytes;

		return userPtr;
	}

	void Free(void* ptr, size_t size) {(void)ptr; (void)size;}

	void Reset() {mRest.Begin = mStart;}

private:
	byte* mStart;
	Span<byte> mRest;
	size_t mAlignment;
};

}}

INTRA_WARNING_POP
