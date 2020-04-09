#pragma once

#include "Intra/Math/Math.h"
#include "Intra/Type.h"
#include "Intra/Range/Concepts.h"
#include "Intra/Range/Operations.h"

INTRA_BEGIN
INTRA_IGNORE_WARNING_COPY_IMPLICITLY_DELETED
template<typename... RANGES> struct RChain;
template<typename R0, typename R1, typename... RANGES> struct RChain<R0, R1, RANGES...>
{
private:
	using ReturnValueType = TCommonRef<TReturnValueTypeOf<R0>,
		TReturnValueTypeOf<R1>, TReturnValueTypeOf<RANGES>...>;
	using NextChain = TSelect<RChain<R1, RANGES...>, R1, (sizeof...(RANGES) > 0)>;
public:
	static constexpr bool IsAnyInstanceFinite = (CFiniteInputRange<R0> && CFiniteInputRange<R1> && CFiniteInputRange<RANGES> && ...),
		IsAnyInstanceInfinite = (CInfiniteInputRange<R0> || CInfiniteInputRange<R1> || CInfiniteInputRange<RANGES> || ...)>;

	constexpr RChain(R0&& r0, R1&& r1, RANGES&&... ranges):
		mRange0(Forward<R0>(r0)), mNext(Forward<R1>(r1), Forward<RANGES>(ranges)...) {}

	[[nodiscard]] constexpr ReturnValueType First() const
	{return mRange0.Empty()? mNext.First(): mRange0.First();}

	constexpr void PopFirst()
	{
		if(!mRange0.Empty()) mRange0.PopFirst();
		else mNext.PopFirst();
	}

	[[nodiscard]] constexpr bool Empty() const
	{return mRange0.Empty() && mNext.Empty();}

	[[nodiscard]] constexpr ReturnValueType Last() const
	{return mNext.Empty()? mRange0.Last(): mNext.Last();}

	constexpr void PopLast()
	{
		if(mNext.Empty()) mRange0.PopLast();
		else mNext.PopLast();
	}


	[[nodiscard]] constexpr index_t Length() const
	{return mRange0.Length() + mNext.Length();}


	[[nodiscard]] constexpr ReturnValueType operator[](Index index) const
	{
		const auto len = mRange0.Length();
		if(index_t(index) < len) return mRange0[index];
		return mNext[index_t(index)-len];
	}

	//TODO: PopFirstCount/PopLastCount

private:
	R0 mRange0;
	NextChain mNext;

	constexpr RChain(decltype(null), R0&& r0, NextChain&& ranges):
		mRange0(Forward<R0>(r0)), mNext(Move(ranges)) {}
};

template<typename R0> struct RChain<R0>
{
private:
	using ReturnValueType = TReturnValueTypeOf<R0>;
public:
	static constexpr bool IsAnyInstanceFinite = CFiniteRange<R0>,
		IsAnyInstanceInfinite = CInfiniteRange<R0>;

	constexpr RChain(const R0& r0): mRange0(r0) {}
	constexpr RChain(R0&& r0): mRange0(Move(r0)) {}

	[[nodiscard]] constexpr ReturnValueType First() const
	{
		INTRA_PRECONDITION(!mRange0.Empty());
		return mRange0.First();
	}

	constexpr void PopFirst()
	{
		INTRA_PRECONDITION(!mRange0.Empty());
		mRange0.PopFirst();
	}

	[[nodiscard]] constexpr bool Empty() const {return mRange0.Empty();}

	[[nodiscard]] constexpr ReturnValueType Last() const {return mRange0.Last();}

	constexpr void PopLast() {mRange0.PopLast();}

	[[nodiscard]] constexpr index_t Length() const {return mRange0.Length();}

	[[nodiscard]] constexpr ReturnValueType operator[](Index index) const {return mRange0[index];}

private:
	R0 mRange0;
};

template<typename R0> [[nodiscard]] constexpr Requires<
	CAsAccessibleRange<R0>,
TRangeOf<R0>> Chain(R0&& r0)
{return {ForwardAsRange<R0>(r0)};}

template<typename R0, typename R1, typename... Rs> [[nodiscard]] constexpr Requires<
	CAsAccessibleRange<R0> && CAsAccessibleRange<R1> && VAll(CAsAccessibleRangeT<Rs>...),
RChain<TRangeOf<R0>, TRangeOf<R1>, TRangeOf<Rs>...>> Chain(R0&& r0, R1&& r1, Rs&&... ranges)
{return {ForwardAsRange<R0>(r0), ForwardAsRange<R1>(r1), ForwardAsRange<Rs>(ranges)...};}
INTRA_END
