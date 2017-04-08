﻿#pragma once

#include "Range/ForwardDecls.h"
#include "Range/Concepts.h"
#include "Range/AsRange.h"
#include "Utils/Optional.h"
#include "Utils/Method.h"

namespace Intra { namespace Range {

INTRA_WARNING_PUSH
INTRA_DISABLE_REDUNDANT_WARNINGS
INTRA_WARNING_DISABLE_COPY_IMPLICITLY_DELETED

template<typename R, typename F> struct RMap:
	Meta::CopyableIf<Meta::IsCopyConstructible<R>::_ && Meta::IsCopyConstructible<F>::_>
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


	forceinline return_value_type First() const
	{return Function()(OriginalRange.First());}

	forceinline void PopFirst()
	{OriginalRange.PopFirst();}

	forceinline bool Empty() const
	{return OriginalRange.Empty() || Function==null;}

	forceinline return_value_type Last() const
	{return Function()(OriginalRange.Last());}

	forceinline void PopLast() {OriginalRange.PopLast();}

	forceinline return_value_type operator[](size_t index) const
	{return Function()(OriginalRange[index]);}

	template<typename U=R> forceinline Meta::EnableIf<
		HasSlicing<U>::_,
	RMap<SliceTypeOf<U>, F>> operator()(size_t start, size_t end) const
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


template<typename R, typename F, typename AsR=AsRangeResult<R>, typename T=ReturnValueTypeOf<AsR>> forceinline Meta::EnableIf<
	IsConsumableRange<AsR>::_ &&
	Meta::IsCallable<F, T>::_,
RMap<Meta::RemoveConstRef<AsR>, F>> Map(R&& range, F func)
{return {Range::Forward<R>(range), Meta::Move(func)};}

template<typename R, typename RET, typename AsR=AsRangeResult<R>, typename T=ValueTypeOf<AsR>> forceinline Meta::EnableIf<
	IsConsumableRange<AsR>::_,
RMap<Meta::RemoveConstRef<AsR>, Utils::ConstMethodWrapper<T, RET>>> Map(R&& range, RET(T::*func)() const)
{return {Range::Forward<R>(range), Utils::Method(func)};}

INTRA_WARNING_POP

}}
