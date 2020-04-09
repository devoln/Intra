#pragma once

#include "Intra/Type.h"
#include "Intra/Range/Span.h"
#include "Intra/Range/StringView.h"
#include "Intra/Range/Stream/RawRead.h"

INTRA_BEGIN
template<typename T> class FlatArrayOfArraysRange
{
	static_assert(CTriviallyCopyable<T>, "T must be trivial.");
public:
	typedef TSelect<GenericStringView<T>, Span<T>, CChar<T>> ArrayOrStringView;
	
	constexpr FlatArrayOfArraysRange(const void* data, size_t size): mData(static_cast<const byte*>(data), size) {}
	constexpr FlatArrayOfArraysRange(CSpan<byte> data): mData(data) {}

	[[nodiscard]] ArrayOrStringView First() const
	{
		auto dataCopy = mData;
		const size_t count = ParseVarUInt(dataCopy);
		return dataCopy.ReinterpretUnsafe<T>().Take(count);
	}

	void PopFirst()
	{
		const size_t count = ParseVarUInt(mData);
		mData.PopFirstCount(count*sizeof(T));
	}

	[[nodiscard]] constexpr bool Empty() const {return mData.Empty();}

	[[nodiscard]] constexpr const byte* RawData() const {return mData.Data();}
	[[nodiscard]] constexpr const byte* RawEnd() const {return mData.End;}
	[[nodiscard]] constexpr CSpan<byte> RawRange() const {return mData;}

private:
	CSpan<byte> mData;
};

typedef FlatArrayOfArraysRange<char> FlatStringRange;
typedef FlatArrayOfArraysRange<const char> FlatConstStringRange;
INTRA_END
