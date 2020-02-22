#pragma once

#include "Core/Range/Concepts.h"
#include "Core/Range/TupleOperation.h"
#include "Core/Numeric.h"

INTRA_BEGIN
INTRA_WARNING_DISABLE_ASSIGN_IMPLICITLY_DELETED

//TODO: optimize

template<typename R0, typename... RANGES> struct RRoundRobin
{
private:
	enum: size_t {EmptyCounterValue = LMaxOf(size_t())};
public:
	enum: bool {RangeIsFinite = CAll<CFiniteRangeT, R0, RANGES...>};

	constexpr forceinline RRoundRobin(null_t=null):
		mCounter(EmptyCounterValue) {}

	constexpr forceinline RRoundRobin(R0&& r0, RANGES&&... ranges):
		mRange0(Forward<R0>(r0)), mNext(Forward<RANGES>(ranges)...),
		mCounter(r0.Empty()? EmptyCounterValue: 0) {}

	constexpr TCommonRef<TReturnValueTypeOf<R0>, TReturnValueTypeOf<RANGES>...> First() const
	{
		return (!mNext.before_(mCounter) && !mRange0.Empty())?
			mRange0.First(): mNext.First();
	}

	constexpr void PopFirst()
	{
		if(!mNext.before_(mCounter))
		{
			mCounter++;
			mRange0.PopFirst();
			if(mRange0.Empty())
				mCounter = EmptyCounterValue;
			return;
		}
		mNext.PopFirst();
	}

	INTRA_NODISCARD constexpr forceinline bool Empty() const {return mRange0.Empty() && mNext.Empty();}

	template<typename = Requires<
		CAll<CHasLengthT, R0, RANGES...>
	>> INTRA_NODISCARD constexpr forceinline index_t Length() const {return mRange0.Length() + mNext.Length();}

	constexpr forceinline bool before_(size_t prevCounter) const
	{return mCounter < prevCounter || mNext.before_(prevCounter);}
private:
	R0 mRange0;
	RRoundRobin<RANGES...> mNext;
	size_t mCounter;
};

template<typename R0> struct RRoundRobin<R0>
{
private:
	enum: size_t {EmptyCounterValue = LMaxOf(size_t())};
public:
	enum: bool {RangeIsFinite = CFiniteRange<R0>};

	constexpr forceinline RRoundRobin(null_t=null): mCounter(EmptyCounterValue) {}

	constexpr forceinline RRoundRobin(R0&& r0):
		mRange0(Forward<R0>(r0)), mCounter(r0.Empty()? EmptyCounterValue: 0) {}


	INTRA_NODISCARD constexpr forceinline TReturnValueTypeOf<R0> First() const
	{
		INTRA_DEBUG_ASSERT(!Empty());
		return mRange0.First();
	}

	constexpr forceinline void PopFirst()
	{
		INTRA_DEBUG_ASSERT(!mRange0.Empty());
		mCounter++;
		mRange0.PopFirst();
		if(mRange0.Empty()) mCounter = EmptyCounterValue;
	}

	constexpr forceinline bool Empty() const {return mRange0.Empty();}
	INTRA_NODISCARD constexpr forceinline index_t Length() const {return mRange0.Length();}

	constexpr forceinline bool before_(size_t prevCounter) const {return mCounter < prevCounter;}
private:

	R0 mRange0;
	size_t mCounter;
};

template<typename R0, typename... RANGES> INTRA_NODISCARD constexpr forceinline
RRoundRobin<TRangeOfTypeNoCRef<R0>, TRangeOfTypeNoCRef<RANGES>...> RoundRobin(R0&& range0, RANGES&&... ranges)
{return {ForwardAsRange<R0>(range0), ForwardAsRange<RANGES>(ranges)...};}
INTRA_END
