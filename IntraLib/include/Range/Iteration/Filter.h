#pragma once

#include "Range/Mixins/RangeMixins.h"
#include "Utils/Optional.h"

namespace Intra { namespace Range {

template<typename R, typename P> struct FilterResult:
	RangeMixin<FilterResult<R, P>, typename R::value_type,
	R::RangeType>=TypeEnum::RandomAccess? TypeEnum::Bidirectional: R::RangeType, R::RangeIsFinite>
{
	typedef typename R::value_type value_type;
	typedef typename R::return_value_type return_value_type;

	forceinline FilterResult(null_t=null): original_range(null), predicate() {}

	forceinline FilterResult(R&& range, P filterPredicate):
		original_range(core::move(range)), predicate(filterPredicate)
		{skip_falses_front(original_range, filterPredicate);}

	forceinline FilterResult(const R& range, P filterPredicate):
		original_range(range), predicate(filterPredicate)
		{skip_falses_front(original_range, filterPredicate);}

	forceinline return_value_type First() const
	{
		INTRA_ASSERT(!Empty());
		return original_range.First();
	}

	forceinline void PopFirst()
	{
		original_range.PopFirst();
		skip_falses_front(original_range, predicate());
	}

	template<typename U=R> Meta::EnableIf<
		IsBidirectionalRange<U>::_,
	return_value_type> Last() const
	{
		INTRA_ASSERT(!Empty());
		auto&& b = original_range.Last();
		if(predicate()(b)) return b;

		auto copy = original_range;
		copy.PopLast();
		skip_falses_back(copy, predicate());
		return copy.Last();
	}

	template<typename U=R> forceinline Meta::EnableIf<
		IsBidirectionalRange<U>::_
	> PopLast()
	{
		skip_falses_back(original_range, predicate());
		original_range.PopLast();
	}

	forceinline bool Empty() const {return original_range.Empty() || predicate==null;}

	//TODO BUG: FilterResults with same range but different predicates are considered equal!
	forceinline bool operator==(const FilterResult& rhs) const {return original_range==rhs.original_range;}

private:
	static void forceinline skip_falses_front(R& original_range, const P& p)
	{
		while(!original_range.Empty() && !p(original_range.First())) original_range.PopFirst();
	}

	static void forceinline skip_falses_back(R& original_range, const P& p)
	{
		while(!original_range.Empty() && !p(original_range.Last())) original_range.PopLast();
	}

	R original_range;
	Utils::Optional<P> predicate;
};

template<typename R, typename P> forceinline Meta::EnableIf<
	!Meta::IsReference<R>::_ && IsInputRange<R>::_,
FilterResult<R, P>> Filter(R&& range, P predicate)
{
	return FilterResult<R, P>(core::move(range), predicate);
}

template<typename R, typename P> forceinline Meta::EnableIf<
	IsForwardRange<R>::_,
FilterResult<R, P>> Filter(const R& range, P predicate)
{
	return FilterResult<R, P>(range, predicate);
}

}}
