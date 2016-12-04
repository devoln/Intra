#pragma once

#include "Range/Mixins/RangeMixins.h"
#include "Utils/Optional.h"

namespace Intra { namespace Range
{

template<typename R, typename F> struct MapResult:
	RangeMixin<MapResult<R, F>,
		Meta::ResultOf<F, typename R::value_type>,
		R::RangeType, R::RangeIsFinite>
{
	typedef Meta::ResultOf<F, typename R::value_type> value_type;
	typedef Meta::ResultOf<F, typename R::return_value_type> return_value_type;

	forceinline MapResult(null_t=null):
		OriginalRange(null), Function() {}

	forceinline MapResult(R&& range, F func):
		OriginalRange(core::move(range)), Function(func) {}

	forceinline MapResult(const R& range, F func):
		OriginalRange(range), Function(func) {}

	forceinline return_value_type First() const
	{return Function()(OriginalRange.First());}

	forceinline void PopFirst()
	{OriginalRange.PopFirst();}

	forceinline bool Empty() const
	{return OriginalRange.Empty() || Function==null;}

	template<typename U=R> forceinline Meta::EnableIf<
		IsBidirectionalRange<U>::_,
	return_value_type> Last() const
	{return Function()(OriginalRange.Last());}

	template<typename U=R> forceinline Meta::EnableIf<
		IsBidirectionalRange<U>::_
	> PopLast() {OriginalRange.PopLast();}

	template<typename U=R> forceinline Meta::EnableIf<
		IsRandomAccessRange<U>::_,
	return_value_type> operator[](size_t index) const
	{return Function()(OriginalRange[index]);}

	template<typename U=R> forceinline Meta::EnableIf<
		HasLength<U>::_,
	size_t> Length() const
	{return OriginalRange.Length();}

	//TODO BUG: MapResults with same range but different functions are considered to be equal!
	forceinline bool operator==(const MapResult& rhs) const
	{return OriginalRange==rhs.OriginalRange;}

	R OriginalRange;
	Utils::Optional<F> Function;
};

template<typename R, typename F> forceinline Meta::EnableIf<
	!Meta::IsReference<R>::_ && IsInputRange<R>::_,
MapResult<R, F>> Map(R&& range, F func)
{return MapResult<R, F>(core::move(range), func);}

template<typename R, typename F> forceinline Meta::EnableIf<
	IsForwardRange<R>::_,
MapResult<R, F>> Map(const R& range, F func)
{return MapResult<R, F>(range, func);}

}}
