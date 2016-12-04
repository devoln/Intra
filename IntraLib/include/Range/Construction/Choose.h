#pragma once

#include "Range/Mixins/RangeMixins.h"

namespace Intra { namespace Range {

template<typename R1, typename R2> struct ChooseResult:
	CommonAllFiniteRangeMixin<ChooseResult<R1, R2>, R1, R2>
{
	typedef Meta::CommonType<typename R1::value_type, typename R2::value_type> value_type;
	typedef Meta::CommonTypeRef<typename R1::return_value_type, typename R2::return_value_type> return_value_type;

	ChooseResult(null_t=null): range1(null), range2(null) {}
	ChooseResult(R1&& ifFalseRange, R2&& ifTrueRange, bool condition):
		range1(condition? null: core::forward<R1>(ifFalseRange)),
		range2(condition? core::forward<R2>(ifTrueRange): null) {}

	forceinline bool Empty() const
	{
		return range1.Empty() && range2.Empty();
	}

	forceinline return_value_type First() const
	{
		return range1.Empty()? range2.First(): range1.First();
	}

	forceinline void PopFirst()
	{
		if(!range1.Empty())
		{
			range1.PopFirst();
			return;
		}
		INTRA_ASSERT(!Empty());
		range2.PopFirst();
	}

	template<typename U=R1> forceinline Meta::EnableIf<
		IsBidirectionalRange<U>::_ && IsBidirectionalRange<R2>::_,
	return_value_type> Last() const
	{
		return range1.Empty()? range2.Last(): range1.Last();
	}

	template<typename U=R1> forceinline Meta::EnableIf<
		IsBidirectionalRange<U>::_ && IsBidirectionalRange<R2>::_
	> PopLast()
	{
		if(!range1.Empty())
		{
			range1.PopLast();
			return;
		}
		INTRA_ASSERT(!Empty());
		range2.PopLast();
	}

	template<typename U=R1> forceinline Meta::EnableIf<
		IsRandomAccessRange<U>::_ && IsRandomAccessRange<R2>::_,
	return_value_type> operator[](size_t index) const
	{
		return range1.Empty()? range2[index]: range1[index];
	}

	template<typename U=R1> forceinline Meta::EnableIf<
		IsFiniteRandomAccessRange<U>::_ && IsFiniteRandomAccessRange<R2>::_,
	ChooseResult> opSlice(size_t start, size_t end) const
	{
		return ChooseResult(
			range1.Empty()? null: range1.opSlice(start, end),
			range1.Empty()? null: range2.opSlice(start, end));
	}

	forceinline bool operator==(const ChooseResult& rhs) const
	{
		return range1==rhs.range1 && range2==rhs.range2;
	}

private:
	R1 range1;
	R2 range2;
};

template<typename R1, typename R2> forceinline
ChooseResult<R1, R2> Choose(R1&& ifFalseRange, R2&& ifTrueRange, bool condition)
{
	return ChooseResult<R1, R2>(core::forward<R1>(ifFalseRange), core::forward<R2>(ifTrueRange), condition);
}

}}
