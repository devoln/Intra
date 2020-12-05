#pragma once

#include "Intra/Range/Search/Single.h"
#include "Intra/Range/StringView.h"

INTRA_BEGIN
template<typename T> class GenericZStringRange
{
public:
	GenericZStringRange() = default;
	constexpr GenericZStringRange(decltype(null)) {}

	/// @param zstrings String of the form "string1\0string2\0string3"
	constexpr GenericZStringRange(GenericStringView<T> zstrings):
		mData(zstrings), mFirstLen(CountUntilAdvance(mData, '\0')) {}

	[[nodiscard]] constexpr GenericStringView<T> First() const {return GenericStringView<T>(mData.Data()-mFirstLen, mData.Data());}
	constexpr void PopFirst() {mFirstLen = CountUntilAdvance(mData, '\0');}
	[[nodiscard]] constexpr bool Empty() const {return mData.Empty() && mFirstLen == 0;}

	//TODO: implement Last and PopLast to make it bidirectional range
	
private:
	/// If there is next element, ``mData``.First() points to '\0'.
	/// If there are no more elements, then mData.Empty()
	GenericStringView<T> mData;

	/// Current string length
	size_t mFirstLen = 0;
};

typedef GenericZStringRange<char> ZStringRange;
INTRA_END
