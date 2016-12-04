#pragma once

#include "Range/Mixins/RangeMixins.h"

namespace Intra { namespace Range {

template<typename Rs> struct FirstTransversalResult:
	RangeMixin<FirstTransversalResult<Rs>, typename Rs::value_type::value_type,
		(Rs::RangeType>TypeEnum::Bidirectional)? TypeEnum::Bidirectional: Rs::RangeType, Rs::RangeIsFinite>
{
	typedef typename Rs::value_type::value_type value_type;
	typedef typename Rs::value_type::return_value_type return_value_type;

	forceinline FirstTransversalResult(null_t=null) {}

	forceinline FirstTransversalResult(Rs&& rangeOfRanges):
		ranges(core::move(rangeOfRanges)) {skip_empty();}

	forceinline FirstTransversalResult(const Rs& rangeOfRanges):
		ranges(rangeOfRanges) {skip_empty();}

	forceinline bool Empty() const {return ranges.Empty();}

	forceinline return_value_type First() const {return ranges.First().First();}

	forceinline void PopFirst()
	{
		ranges.PopFirst();
		skip_empty();
	}

	template<typename U=Rs> forceinline Meta::EnableIf<
		IsBidirectionalRange<U>::_,
	return_value_type> Last() const
	{
		while(ranges.Last().Empty()) ranges.PopLast();
		return ranges.Last().First();
	}

	template<typename U=Rs> forceinline Meta::EnableIf<
		IsBidirectionalRange<U>::_
	> PopLast()
	{
		INTRA_ASSERT(!Empty());
		while(ranges.Last().Empty()) ranges.PopLast();
		ranges.PopLast();
	}

	forceinline bool operator==(const FirstTransversalResult& rhs) const {return ranges==rhs.ranges;}

private:
	Rs ranges;

	void skip_empty()
	{
		while(!Empty() && ranges.First().Empty()) ranges.PopFirst();
	}
};

template<typename R> forceinline Meta::EnableIf<
	!Meta::IsReference<R>::_ && IsRangeOfRanges<R>::_,
FirstTransversalResult<R>> FirstTransversal(R&& range) {return FirstTransversalResult<R>(core::move(range));}

template<typename R> forceinline Meta::EnableIf<
	IsRangeOfRanges<R>::_ && IsForwardRange<R>::_,
FirstTransversalResult<R>> FirstTransversal(const R& range) {return FirstTransversalResult<R>(range);}

}}
