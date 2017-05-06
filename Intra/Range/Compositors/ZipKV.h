#pragma once

#include "Concepts/Range.h"
#include "Concepts/RangeOf.h"
#include "Meta/Pair.h"
#include "Utils/Op.h"


INTRA_WARNING_PUSH
INTRA_DISABLE_REDUNDANT_WARNINGS
INTRA_WARNING_DISABLE_COPY_IMPLICITLY_DELETED

namespace Intra { namespace Range {

template<typename KR, typename VR> struct RZipKV
{
private:
	typedef KeyValuePair<Concepts::ReturnValueTypeOf<KR>, Concepts::ReturnValueTypeOf<VR>> ReturnValueType;
	typedef KeyValuePair<KR, VR> OriginalRangeTuple;
public:
	OriginalRangeTuple OriginalRanges;

	forceinline RZipKV(null_t=null) {}

	forceinline RZipKV(KR&& keys, VR&& values):
		OriginalRanges(Cpp::Move(keys), Cpp::Move(values)) {}
	
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
		Concepts::HasLast<KU>::_ &&
		Concepts::HasLast<VR>::_,
	ReturnValueType> Last() const
	{return {OriginalRanges.Key.Last(), OriginalRanges.Value.Last()};}
	
	template<typename KU=KR> forceinline Meta::EnableIf<
		Concepts::HasPopLast<KU>::_ &&
		Concepts::HasPopLast<VR>::_
	> PopLast()
	{
		OriginalRanges.Key.PopLast();
		OriginalRanges.Value.PopLast();
	}


	template<typename KU=KR> forceinline Meta::EnableIf<
		Concepts::HasLength<KU>::_ &&
		Concepts::HasLength<VR>::_,
	size_t> Length() const
	{return Op::Min(OriginalRanges.Key.Length(), OriginalRanges.Value.Length());}

	template<typename KU=KR> forceinline Meta::EnableIf<
		Concepts::HasIndex<KU>::_ &&
		Concepts::HasIndex<VR>::_,
	ReturnValueType> operator[](size_t index) const
	{return {OriginalRanges.Key[index], OriginalRanges.Value[index]};}

	template<typename KU=KR> forceinline Meta::EnableIf<
		Concepts::HasSlicing<KU>::_ &&
		Concepts::HasSlicing<VR>::_,
	RZipKV> operator()(size_t startIndex, size_t endIndex) const
	{return {OriginalRanges.Key(startIndex, endIndex), OriginalRanges.Value(startIndex, endIndex)};}
};

template<typename KR, typename VR, typename=Meta::EnableIf<
	Concepts::IsAsAccessibleRange<KR>::_ &&
	Concepts::IsAsAccessibleRange<VR>::_
>> forceinline RZipKV<Concepts::RangeOfTypeNoCRef<KR>, Concepts::RangeOfTypeNoCRef<VR>> ZipKV(KR&& keys, VR&& values)
{return {Range::Forward<KR>(keys), Range::Forward<VR>(values)};}

}}

INTRA_WARNING_POP
