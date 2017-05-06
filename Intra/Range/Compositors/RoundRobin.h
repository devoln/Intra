#pragma once

#include "Concepts/Range.h"
#include "Range/TupleOperation.h"

namespace Intra { namespace Range {

INTRA_WARNING_PUSH
INTRA_DISABLE_REDUNDANT_WARNINGS
INTRA_WARNING_DISABLE_COPY_IMPLICITLY_DELETED

template<typename R0, typename... RANGES> struct RRoundRobin
{
	enum: bool {RangeIsFinite = Concepts::CommonRangeCategoryAllFinite<R0, RANGES...>::Finite};

	forceinline RRoundRobin(null_t=null):
		mCounter(Meta::NumericLimits<decltype(mCounter)>::Max()) {}

	forceinline RRoundRobin(R0&& r0, RANGES&&... ranges):
		mRange0(Cpp::Forward<R0>(r0)), mNext(Cpp::Forward<RANGES>(ranges)...),
		mCounter(r0.Empty()? Meta::NumericLimits<decltype(mCounter)>::Max(): 0) {}

	Meta::CommonTypeRef<Concepts::ReturnValueTypeOf<R0>, Concepts::ReturnValueTypeOf<RANGES>...> First() const
	{
		return (!mNext.before_(mCounter) && !mRange0.Empty())?
			mRange0.First(): mNext.First();
	}

	void PopFirst()
	{
		if(!mNext.before_(mCounter))
		{
			mCounter++;
			mRange0.PopFirst();
			if(mRange0.Empty())
				mCounter = Meta::NumericLimits<decltype(mCounter)>::Max();
			return;
		}
		mNext.PopFirst();
	}

	forceinline bool Empty() const {return mRange0.Empty() && mNext.Empty();}

	forceinline bool operator==(const RRoundRobin& rhs) const
	{
		return ((mRange0.Empty() && rhs.mRange0.Empty()) ||
			(mRange0==rhs.mRange0 && mCounter==rhs.mCounter)) && mNext==rhs.mNext;
	}

	forceinline bool operator!=(const RRoundRobin& rhs) const
	{return !operator==(rhs);}

	forceinline size_t Length() const {return mRange0.Length()+mNext.Length();}

	forceinline bool before_(size_t prevCounter) const
	{return mCounter<prevCounter || mNext.before_(prevCounter);}

private:
	R0 mRange0;
	RRoundRobin<RANGES...> mNext;
	size_t mCounter;
};

template<typename R0> struct RRoundRobin<R0>
{
	enum: bool {RangeIsFinite = Concepts::IsFiniteRange<R0>::_};

	forceinline RRoundRobin(null_t=null):
		mCounter(Meta::NumericLimits<decltype(mCounter)>::Max()) {}

	forceinline RRoundRobin(R0&& r0):
		mRange0(Cpp::Forward<R0>(r0)), mCounter(r0.Empty()? Meta::NumericLimits<decltype(mCounter)>::Max(): 0) {}


	forceinline Concepts::ReturnValueTypeOf<R0> First() const
	{
		INTRA_DEBUG_ASSERT(!Empty());
		return mRange0.First();
	}

	forceinline void PopFirst()
	{
		INTRA_DEBUG_ASSERT(!mRange0.Empty());
		mCounter++;
		mRange0.PopFirst();
		if(mRange0.Empty())
			mCounter = Meta::NumericLimits<decltype(mCounter)>::Max();
	}

	forceinline bool Empty() const {return mRange0.Empty();}

	forceinline bool operator==(const RRoundRobin& rhs) const
	{
		return (mRange0.Empty() && rhs.mRange0.Empty()) ||
			(mRange0==rhs.mRange0 && mCounter==rhs.mCounter);
	}

	forceinline size_t Length() const {return mRange0.Length();}

	forceinline bool before_(size_t prevCounter) const {return mCounter<prevCounter;}

private:
	R0 mRange0;
	size_t mCounter;
};

INTRA_WARNING_POP

template<typename R0, typename... RANGES> forceinline
RRoundRobin<Concepts::RangeOfTypeNoCRef<R0>, Concepts::RangeOfTypeNoCRef<RANGES>...> RoundRobin(R0&& range0, RANGES&&... ranges)
{return {Range::Forward<R0>(range0), Range::Forward<RANGES>(ranges)...};}


}}
