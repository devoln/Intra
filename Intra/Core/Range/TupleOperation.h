#pragma once

#include "Core/Functional.h"
#include "Core/Tuple.h"
#include "Core/Range/Concepts.h"

INTRA_CORE_RANGE_BEGIN
template<typename Tuple1, typename Tuple2, size_t... I>
INTRA_CONSTEXPR2 bool TupleEqual(const Tuple1& lhs, const Tuple2& rhs)
{
	bool result = true;
	InitializerList<bool>{(result = result && GetField<I>(lhs) == GetField<I>(rhs))...};
	return result;
}

template<typename R0> bool AnyEmpty(const Tuple<R0>& ranges)
{return ranges.first.Empty();}

template<typename R0, typename R1, typename... Rs>
bool AnyEmpty(const Tuple<R0, R1, Rs...>& ranges)
{return ranges.first.Empty() || AnyEmpty(ranges.next);}



template<typename R0> bool AllEmpty(const Tuple<R0>& ranges)
{return ranges.first.Empty();}

template<typename R0, typename R1, typename... Rs>
bool AllEmpty(const Tuple<R0, R1, Rs...>& ranges)
{return ranges.first.Empty() && AllEmpty(ranges.next);}


template<typename R0> size_t MinLength(const Tuple<R0>& ranges)
{return ranges.first.Length();}

template<typename R0, typename R1, typename... Rs> size_t MinLength(const Tuple<R0, R1, Rs...>& ranges)
{return FMin(ranges.first.Length(), MinLength(ranges.next));}

struct Fronter
{
	template<typename T> constexpr forceinline TReturnValueTypeOf<T> operator()(T&& range) const
	{return range.First();}
};

struct Backer
{
	template<typename T> constexpr forceinline TReturnValueTypeOf<T> operator()(T&& range) const
	{return range.Last();}
};

struct Indexer
{
	size_t index;
	template<typename T> constexpr forceinline TReturnValueTypeOf<T> operator()(T&& range) const
	{return range[index];}
};

struct Slicer
{
	size_t start, end;
	template<typename T> constexpr forceinline auto operator()(T&& range) const {return range(start, end);}
};

struct PopFronter
{
	template<typename T> INTRA_CONSTEXPR2 forceinline void operator()(T& range) const {return range.PopFirst();}
};

struct PopBacker
{
	template<typename T> INTRA_CONSTEXPR2 forceinline void operator()(T& range) const {return range.PopLast();}
};
INTRA_CORE_RANGE_END
