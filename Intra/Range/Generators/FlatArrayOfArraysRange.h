#pragma once

#include "Meta/Type.h"
#include "Utils/Span.h"
#include "Utils/StringView.h"
#include "Range/Stream/RawRead.h"

namespace Intra { namespace Range {

template<typename T> class FlatArrayOfArraysRange
{
	static_assert(Meta::IsTriviallySerializable<T>::_, "T must be trivial.");
public:
	typedef Meta::SelectType<GenericStringView<T>, Span<T>, Meta::IsCharType<T>::_> ArrayOrStringView;
	
	forceinline FlatArrayOfArraysRange(const void* data, size_t size): mData(static_cast<const byte*>(data), size) {}
	forceinline FlatArrayOfArraysRange(CSpan<byte> data): mData(data) {}

	ArrayOrStringView First() const
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

	forceinline bool Empty() const {return mData.Empty();}

	forceinline const byte* RawData() const {return mData.Data();}
	forceinline const byte* RawEnd() const {return mData.End;}
	forceinline CSpan<byte> RawRange() const {return mData;}

private:
	CSpan<byte> mData;
};

typedef FlatArrayOfArraysRange<char> FlatStringRange;
typedef FlatArrayOfArraysRange<const char> FlatConstStringRange;

}}
