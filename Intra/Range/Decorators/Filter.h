#pragma once

#include "Range/ForwardDecls.h"
#include "Concepts/Range.h"
#include "Utils/Optional.h"

namespace Intra { namespace Range {

INTRA_WARNING_PUSH
INTRA_DISABLE_REDUNDANT_WARNINGS
INTRA_WARNING_DISABLE_COPY_IMPLICITLY_DELETED

template<typename R, typename P> struct RFilter
{
	enum: bool
	{
		RangeIsFinite = Concepts::IsFiniteRange<R>::_,
		RangeIsInfinite = Concepts::IsInfiniteRange<R>::_
	};

	forceinline RFilter(null_t=null): mOriginalRange(null), mPredicate() {}

	forceinline RFilter(R&& range, P filterPredicate):
		mOriginalRange(Cpp::Move(range)), mPredicate(filterPredicate)
	{skip_falses_front(mOriginalRange, filterPredicate);}

	forceinline RFilter(const R& range, P filterPredicate):
		mOriginalRange(range), mPredicate(filterPredicate)
	{skip_falses_front(mOriginalRange, filterPredicate);}


	forceinline Concepts::ReturnValueTypeOf<R> First() const
	{
		INTRA_DEBUG_ASSERT(!Empty());
		return mOriginalRange.First();
	}

	forceinline void PopFirst()
	{
		mOriginalRange.PopFirst();
		skip_falses_front(mOriginalRange, mPredicate());
	}

	template<typename U=R> Meta::EnableIf<
		Concepts::HasLast<U>::_ &&
		Concepts::HasPopLast<U>::_,
	Concepts::ReturnValueTypeOf<R>> Last() const
	{
		INTRA_DEBUG_ASSERT(!Empty());
		auto&& b = mOriginalRange.Last();
		if(mPredicate()(b)) return b;

		auto copy = mOriginalRange;
		copy.PopLast();
		skip_falses_back(copy, mPredicate());
		return copy.Last();
	}

	template<typename U=R> forceinline Meta::EnableIf<
		Concepts::HasLast<U>::_ &&
		Concepts::HasPopLast<U>::_
	> PopLast()
	{
		skip_falses_back(mOriginalRange, mPredicate());
		mOriginalRange.PopLast();
	}

	forceinline bool Empty() const {return mOriginalRange.Empty() || mPredicate==null;}

	//TODO BUG: FilterResults with same range but different predicates are considered equal!
	forceinline bool operator==(const RFilter& rhs) const {return mOriginalRange==rhs.mOriginalRange;}

private:
	static void forceinline skip_falses_front(R& originalRange, const P& p)
	{while(!originalRange.Empty() && !p(originalRange.First())) originalRange.PopFirst();}

	static void forceinline skip_falses_back(R& originalRange, const P& p)
	{while(!originalRange.Empty() && !p(originalRange.Last())) originalRange.PopLast();}

	R mOriginalRange;
	Utils::Optional<P> mPredicate;
};

INTRA_WARNING_POP

template<typename R, typename P> forceinline Meta::EnableIf<
	Concepts::IsAsConsumableRange<R>::_,
RFilter<Meta::RemoveConstRef<Concepts::RangeOfType<R>>, P>> Filter(R&& range, P predicate)
{return {Cpp::Forward<R>(range), predicate};}

}}
