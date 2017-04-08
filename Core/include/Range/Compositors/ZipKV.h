#pragma once

#include "Range/Concepts.h"
#include "Range/AsRange.h"
#include "Meta/Pair.h"
#include "Algo/Op.h"


INTRA_WARNING_PUSH
INTRA_DISABLE_REDUNDANT_WARNINGS
INTRA_WARNING_DISABLE_COPY_IMPLICITLY_DELETED

namespace Intra { namespace Range {

template<typename KR, typename VR> struct RZipKV
{
private:
	typedef KeyValuePair<ReturnValueTypeOf<KR>, ReturnValueTypeOf<VR>> ReturnValueType;
	typedef KeyValuePair<KR, VR> OriginalRangeTuple;
public:
	OriginalRangeTuple OriginalRanges;

	forceinline RZipKV(null_t=null) {}

	forceinline RZipKV(KR&& keys, VR&& values):
		OriginalRanges(Meta::Move(keys), Meta::Move(values)) {}
	
	forceinline RZipKV(OriginalRangeTuple ranges): OriginalRanges(ranges) {}


	forceinline ReturnValueType First() const
	{return {OriginalRanges.Key.First(), OriginalRanges.Value.First()};}
	
	forceinline void PopFirst()
	{
		OriginalRanges.Key.PopFirst();
		OriginalRanges.Value.PopFirst();
	}
	
	forceinline bool Empty() const
	{return OriginalRanges.Key.Empty() || OriginalRanges.Value.Empty();}


	template<typename KU=KR> forceinline Meta::EnableIf<
		HasLast<KU>::_ && HasLast<VR>::_,
	ReturnValueType> Last() const
	{return {OriginalRanges.Key.Last(), OriginalRanges.Value.Last()};}
	
	template<typename KU=KR> forceinline Meta::EnableIf<
		HasPopLast<KU>::_ && HasPopLast<VR>::_
	> PopLast()
	{
		OriginalRanges.Key.PopLast();
		OriginalRanges.Value.PopLast();
	}


	template<typename KU=KR> forceinline Meta::EnableIf<
		HasLength<KU>::_ && HasLength<VR>::_,
	size_t> Length() const
	{return Op::Min(OriginalRanges.Key.Length(), OriginalRanges.Value.Length());}

	template<typename KU=KR> forceinline Meta::EnableIf<
		HasIndex<KU>::_ && HasIndex<VR>::_,
	ReturnValueType> operator[](size_t index) const
	{return {OriginalRanges.Key[index], OriginalRanges.Value[index]};}

	template<typename KU=KR> forceinline Meta::EnableIf<
		HasSlicing<KU>::_ && HasSlicing<VR>::_,
	RZipKV> operator()(size_t startIndex, size_t endIndex) const
	{return {OriginalRanges.Key(startIndex, endIndex), OriginalRanges.Value(startIndex, endIndex)};}
};

template<typename KR, typename VR, typename=Meta::EnableIf<
	IsAsAccessibleRange<KR>::_ && IsAsAccessibleRange<VR>::_
>> forceinline RZipKV<AsRangeResultNoCRef<KR>, AsRangeResultNoCRef<VR>> ZipKV(KR&& keys, VR&& values)
{return {Range::Forward<KR>(keys), Range::Forward<VR>(values)};}

}}

INTRA_WARNING_POP
