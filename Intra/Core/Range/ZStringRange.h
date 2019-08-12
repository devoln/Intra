#pragma once

#include "Core/Range/Search/Single.h"
#include "Core/Range/StringView.h"

INTRA_CORE_RANGE_BEGIN
template<typename T> class GenericZStringRange
{
public:
	GenericZStringRange() = default;
	constexpr forceinline GenericZStringRange(null_t) {}

	//! @param zstrings String of the form "string1\0string2\0string3"
	INTRA_CONSTEXPR2 forceinline GenericZStringRange(GenericStringView<T> zstrings):
		mData(zstrings), mFirstLen(CountUntilAdvance(mData, '\0')) {}

	INTRA_NODISCARD constexpr forceinline GenericStringView<T> First() const {return GenericStringView<T>(mData.Data()-mFirstLen, mData.Data());}
	INTRA_CONSTEXPR2 forceinline void PopFirst() {mFirstLen = CountUntilAdvance(mData, '\0');}
	INTRA_NODISCARD constexpr forceinline bool Empty() const {return mData.Empty() && mFirstLen == 0;}

	//TODO: implement Last and PopLast to make it bidirectional range
	
private:
	//! If there is next element, ``mData``.First() points to '\0'.
	//! If there are no more elements, then mData.Empty()
	GenericStringView<T> mData;

	//! Current string length
	size_t mFirstLen = 0;
};

typedef GenericZStringRange<char> ZStringRange;
INTRA_CORE_RANGE_END
