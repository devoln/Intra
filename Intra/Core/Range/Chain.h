#pragma once

#include "Core/Type.h"
#include "Core/Range/Concepts.h"
#include "Core/Range/TupleOperation.h"

INTRA_CORE_RANGE_BEGIN
INTRA_WARNING_DISABLE_COPY_IMPLICITLY_DELETED
template<typename... RANGES> struct RChain;
template<typename R0, typename R1, typename... RANGES> struct RChain<R0, R1, RANGES...>
{
private:
	typedef TCommonRef<TReturnValueTypeOf<R0>,
		TReturnValueTypeOf<R1>, TReturnValueTypeOf<RANGES>...> ReturnValueType;
	typedef TSelect<RChain<R1, RANGES...>, R1, (sizeof...(RANGES) > 0)> NextChain;
public:
	enum: byte {RangeIsFinite = CAll<CFiniteRangeT, R0, R1, RANGES...>};

	constexpr forceinline RChain(R0&& r0, R1&& r1, RANGES&&... ranges):
		mRange0(Forward<R0>(r0)), mNext(Forward<R1>(r1), Forward<RANGES>(ranges)...) {}

	INTRA_NODISCARD constexpr forceinline ReturnValueType First() const
	{return mRange0.Empty()? mNext.First(): mRange0.First();}

	INTRA_CONSTEXPR2 forceinline void PopFirst()
	{
		if(!mRange0.Empty()) mRange0.PopFirst();
		else mNext.PopFirst();
	}

	INTRA_NODISCARD constexpr forceinline bool Empty() const
	{return mRange0.Empty() && mNext.Empty();}

	INTRA_NODISCARD constexpr forceinline ReturnValueType Last() const
	{return mNext.Empty()? mRange0.Last(): mNext.Last();}

	constexpr forceinline void PopLast()
	{
		if(mNext.Empty()) mRange0.PopLast();
		else mNext.PopLast();
	}


	INTRA_NODISCARD constexpr forceinline index_t Length() const
	{return mRange0.Length() + mNext.Length();}


	INTRA_NODISCARD INTRA_CONSTEXPR2 forceinline ReturnValueType operator[](size_t index) const
	{
		const size_t len = mRange0.Length();
		if(index < len) return mRange0[index];
		return mNext[index-len];
	}

	INTRA_NODISCARD INTRA_CONSTEXPR2 forceinline RChain operator()(size_t startIndex, size_t endIndex) const
	{
		const size_t len = mRange0.Length();
		return {
			mRange0(
				startIndex > len? len: startIndex,
				len > endIndex? endIndex: len),
			mNext(
				startIndex > len? startIndex-len: 0,
				endIndex > len? endIndex-len: 0)
		};
	}

private:
	R0 mRange0;
	NextChain mNext;

	constexpr forceinline RChain(R0&& r0, NextChain&& ranges):
		mRange0(Forward<R0>(r0)), mNext(Move(ranges)) {}
};

template<typename R0> struct RChain<R0>
{
private:
	typedef TReturnValueTypeOf<R0> ReturnValueType;
public:
	enum: bool {RangeIsFinite = CFiniteRange<R0>};

	constexpr forceinline RChain(const R0& r0):
		mRange0(r0) {}

	constexpr forceinline RChain(R0&& r0):
		mRange0(Move(r0)) {}

	INTRA_NODISCARD INTRA_CONSTEXPR2 forceinline ReturnValueType First() const
	{
		INTRA_DEBUG_ASSERT(!mRange0.Empty());
		return mRange0.First();
	}

	INTRA_CONSTEXPR2 forceinline void PopFirst()
	{
		INTRA_DEBUG_ASSERT(!mRange0.Empty());
		mRange0.PopFirst();
	}

	INTRA_NODISCARD constexpr forceinline bool Empty() const {return mRange0.Empty();}

	INTRA_NODISCARD constexpr forceinline ReturnValueType Last() const {return mRange0.Last();}

	INTRA_CONSTEXPR2 forceinline void PopLast() {mRange0.PopLast();}

	INTRA_NODISCARD constexpr forceinline index_t Length() const {return mRange0.Length();}

	INTRA_NODISCARD INTRA_CONSTEXPR2 forceinline ReturnValueType operator[](size_t index) const {return mRange0[index];}

private:
	R0 mRange0;
};

template<typename... RANGES> INTRA_NODISCARD constexpr forceinline Requires<
	CAll<CAsAccessibleRangeT, RANGES...>,
RChain<TRangeOfTypeNoCRef<RANGES>...>> Chain(RANGES&&... ranges)
{return {ForwardAsRange<RANGES>(ranges)...};}
INTRA_CORE_RANGE_END
