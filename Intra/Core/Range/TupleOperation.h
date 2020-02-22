#pragma once

#include "Core/Functional.h"
#include "Core/Tuple.h"
#include "Core/Range/Concepts.h"

INTRA_BEGIN
struct TFFirst
{
	template<typename T> constexpr forceinline decltype(auto) operator()(T&& range) const
	{return range.First();}
};

struct TFLast
{
	template<typename T> constexpr forceinline decltype(auto) operator()(T&& range) const
	{return range.Last();}
};

struct TFSpecificIndex
{
	size_t index;
	template<typename T> constexpr forceinline decltype(auto) operator()(T&& range) const
	{return range[index];}
};

struct TFSlice
{
	size_t start, end;
	template<typename T> constexpr forceinline decltype(auto) operator()(T&& range) const {return range(start, end);}
};

struct TFPopFirst
{
	template<typename T> constexpr forceinline void operator()(T& range) const {range.PopFirst();}
};

struct TFPopLast
{
	template<typename T> constexpr forceinline void operator()(T& range) const {range.PopLast();}
};

struct TFLength
{
	template<typename T> constexpr forceinline index_t operator()(T&& range) const {return range.Length();}
};
INTRA_END
