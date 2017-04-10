#pragma once

#include "Platform/CppFeatures.h"
#include "Algo/Search/Single.h"
#include "Range/Generators/StringView.h"

namespace Intra { namespace Range {

template<typename T> class GenericZStringRange
{
public:
	forceinline GenericZStringRange(null_t=null): mFirstLen(0) {}

	//! @param zstrings Строка вида "строка1\0строка2\0строка3"
	forceinline GenericZStringRange(GenericStringView<T> zstrings):
		mData(zstrings), mFirstLen(Algo::CountUntilAdvance(mData, '\0')) {}

	forceinline GenericStringView<T> First() const {return GenericStringView<T>(mData.Data()-mFirstLen, mData.Data());}
	forceinline void PopFirst() {mFirstLen = Algo::CountUntilAdvance(mData, '\0');}
	forceinline bool Empty() const {return mData.Empty() && mFirstLen==0;}

	//TODO: implement Last and PopLast to make it bidirectional range
	
private:
	//Если есть следующий элемент, начало указывает на '\0'.
	//Если больше элементов нет, mData.Empty()
	GenericStringView<T> mData;

	//Длина текущей строки
	size_t mFirstLen;
};

typedef GenericZStringRange<char> ZStringRange;

}
using Range::ZStringRange;

}
