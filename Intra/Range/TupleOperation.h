#pragma once

#include "Utils/Op.h"
#include "Meta/Tuple.h"
#include "Concepts/Range.h"

namespace Intra { namespace Range {

template<typename R0> bool AnyEmpty(const Meta::Tuple<R0>& ranges)
{return ranges.first.Empty();}

template<typename R0, typename R1, typename... Rs>
bool AnyEmpty(const Meta::Tuple<R0, R1, Rs...>& ranges)
{return ranges.first.Empty() || AnyEmpty(ranges.next);}



template<typename R0> bool AllEmpty(const Meta::Tuple<R0>& ranges)
{return ranges.first.Empty();}

template<typename R0, typename R1, typename... Rs>
bool AllEmpty(const Meta::Tuple<R0, R1, Rs...>& ranges)
{return ranges.first.Empty() && AllEmpty(ranges.next);}


template<typename R0> size_t MinLength(const Meta::Tuple<R0>& ranges)
{return ranges.first.Length();}

template<typename R0, typename R1, typename... Rs> size_t MinLength(const Meta::Tuple<R0, R1, Rs...>& ranges)
{return Op::Min(ranges.first.Length(), MinLength(ranges.next));}


template<typename... Rs> struct AllHasLength;

template<typename R0, typename R1, typename... Rs> struct AllHasLength<R0, R1, Rs...>: Meta::TypeFromValue<bool,
	Concepts::HasLength<R0>::_ &&
	AllHasLength<R1, Rs...>::_
> {};

template<typename R0> struct AllHasLength<R0>: Meta::TypeFromValue<bool,
	Concepts::HasLength<R0>::_
> {};



struct Fronter
{
	template<typename T> forceinline Concepts::ReturnValueTypeOf<T> operator()(const T& range) const
	{return range.First();}
};

struct Backer
{
	template<typename T> forceinline Concepts::ReturnValueTypeOf<T> operator()(const T& range) const
	{return range.Last();}
};

struct Indexer
{
	size_t index;
	template<typename T> forceinline Concepts::ReturnValueTypeOf<T> operator()(const T& range) const
	{return range[index];}
};

struct Slicer
{
	size_t start, end;
	template<typename T> forceinline Concepts::SliceTypeOf<T> operator()(const T& range) const
		{return range(start, end);}
};

struct PopFronter
{
	template<typename T> forceinline void operator()(T& range) const
	{return range.PopFirst();}
};

struct PopBacker
{
	template<typename T> forceinline void operator()(T& range) const
	{return range.PopLast();}
};

}}
