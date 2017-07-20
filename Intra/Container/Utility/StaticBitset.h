#pragma once

#include "Cpp/Fundamental.h"
#include "Utils/Debug.h"

namespace Intra { namespace Container {

template<size_t N> struct StaticBitset
{
	uint Data[(N + 31) >> 5]{};

	bool operator[](size_t index) const
	{
		INTRA_DEBUG_ASSERT(index < N);
		return ((Data[index >> 5] >> (index & 31)) & 1) != 0;
	}

	void Set(size_t index)
	{
		INTRA_DEBUG_ASSERT(index < N);
		Data[index >> 5] |= 1 << (index & 31);
	}

	void Reset(size_t index)
	{
		INTRA_DEBUG_ASSERT(index < N);
		Data[index >> 5] &= ~(1 << (index & 31));
	}
};

}
using Container::StaticBitset;

}
