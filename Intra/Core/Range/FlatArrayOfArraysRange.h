#pragma once

#include "Core/Type.h"
#include "Core/Range/Span.h"
#include "Core/Range/StringView.h"
#include "Core/Range/Stream/RawRead.h"

INTRA_BEGIN
template<typename T> class FlatArrayOfArraysRange
{
	static_assert(CPod<T>, "T must be trivial.");
public:
	typedef TSelect<GenericStringView<T>, Span<T>, CChar<T>> ArrayOrStringView;
	
	constexpr forceinline FlatArrayOfArraysRange(const void* data, size_t size): mData(static_cast<const byte*>(data), size) {}
	constexpr forceinline FlatArrayOfArraysRange(CSpan<byte> data): mData(data) {}

	INTRA_NODISCARD ArrayOrStringView First() const
	{
		auto dataCopy = mData;
		const size_t count = ParseVarUInt(dataCopy);
		return dataCopy.Reinterpret<T>().Take(count);
	}

	void PopFirst()
	{
		const size_t count = ParseVarUInt(mData);
		mData.PopFirstN(count*sizeof(T));
	}

	INTRA_NODISCARD constexpr forceinline bool Empty() const {return mData.Empty();}

	INTRA_NODISCARD constexpr forceinline const byte* RawData() const {return mData.Data();}
	INTRA_NODISCARD constexpr forceinline const byte* RawEnd() const {return mData.End;}
	INTRA_NODISCARD constexpr forceinline CSpan<byte> RawRange() const {return mData;}

private:
	CSpan<byte> mData;
};

typedef FlatArrayOfArraysRange<char> FlatStringRange;
typedef FlatArrayOfArraysRange<const char> FlatConstStringRange;
INTRA_END
