#pragma once

#include "Range/ForwardDecls.h"
#include "Range/Concepts.h"
#include "Utils/Optional.h"

namespace Intra { namespace Range {

INTRA_WARNING_PUSH
INTRA_DISABLE_REDUNDANT_WARNINGS
INTRA_WARNING_DISABLE_COPY_IMPLICITLY_DELETED

template<typename R, typename F> struct RMap
{
private:
	typedef Meta::ResultOf<F, ReturnValueTypeOf<R>> return_value_type;
public:
	enum: bool {RangeIsFinite = IsFiniteRange<R>::_, RangeIsInfinite = IsInfiniteRange<R>::_};

	forceinline RMap(null_t=null):
		OriginalRange(null), Function() {}

	forceinline RMap(R&& range, F func):
		OriginalRange(Meta::Move(range)), Function(func) {}

	forceinline RMap(const R& range, F func):
		OriginalRange(range), Function(func) {}

	//Для совместимости с Visual Studio 2013:
	RMap(const RMap&) = default;
	RMap& operator=(const RMap&) = default;
	forceinline RMap(RMap&& rhs):
		OriginalRange(Meta::Move(rhs.OriginalRange)),
		Function(Meta::Move(rhs.Function)) {}
	forceinline RMap& operator=(RMap&& rhs)
	{
		OriginalRange = Meta::Move(rhs.OriginalRange);
		return *this;
	}


	forceinline return_value_type First() const
	{return Function()(OriginalRange.First());}

	forceinline void PopFirst()
	{OriginalRange.PopFirst();}

	forceinline bool Empty() const
	{return OriginalRange.Empty() || Function==null;}

	template<typename U=R> forceinline Meta::EnableIf<
		HasLast<U>::_,
	return_value_type> Last() const
	{return Function()(OriginalRange.Last());}

	template<typename U=R> forceinline Meta::EnableIf<
		HasPopLast<U>::_
	> PopLast() {OriginalRange.PopLast();}

	template<typename U=R> forceinline Meta::EnableIf<
		HasIndex<U>::_,
	return_value_type> operator[](size_t index) const
	{return Function()(OriginalRange[index]);}

	template<typename U=R> forceinline Meta::EnableIf<
		HasSlicing<U>::_,
	RMap<SliceTypeOf<R>, F>> operator()(size_t start, size_t end) const
	{return {OriginalRange(start, end), Function()};}

	template<typename U=R> forceinline Meta::EnableIf<
		HasLength<U>::_,
	size_t> Length() const
	{return OriginalRange.Length();}

	forceinline bool operator==(const RMap& rhs) const
	{return Function==rhs.Function && OriginalRange==rhs.OriginalRange;}

	R OriginalRange;
	Utils::Optional<F> Function;
};


template<typename R, typename F> forceinline Meta::EnableIf<
	!Meta::IsReference<R>::_ && IsInputRange<R>::_,
RMap<R, F>> Map(R&& range, F func)
{return {Meta::Move(range), Meta::Move(func)};}

template<typename R, typename F> forceinline Meta::EnableIf<
	IsForwardRange<R>::_,
RMap<R, F>> Map(const R& range, F func)
{return {range, Meta::Move(func)};}

template<typename T, size_t N, typename F> forceinline
RMap<AsRangeResult<T(&)[N]>, F> Map(T(&arr)[N], F func)
{return Map(AsRange(arr), Meta::Move(func));}

INTRA_WARNING_POP

}}
