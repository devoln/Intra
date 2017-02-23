#pragma once

#include "Platform/CppFeatures.h"
#include "Platform/CppWarnings.h"
#include "Range/Concepts.h"
#include "Range/AsRange.h"

namespace Intra { namespace Range {

INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS

template<typename R> struct RRetro
{
	R OriginalRange;

	RRetro(null_t=null): OriginalRange(null) {}
	RRetro(R&& range): OriginalRange(Meta::Move(range)) {}
	RRetro(const R& range): OriginalRange(range) {}

	forceinline bool Empty() const {return OriginalRange.Empty();}
	forceinline ReturnValueTypeOf<R> First() const {return OriginalRange.Last();}
	forceinline void PopFirst() {OriginalRange.PopLast();}
	forceinline ReturnValueTypeOf<R> Last() const {return OriginalRange.First();}
	forceinline void PopLast() {OriginalRange.PopFirst();}
	
	ReturnValueTypeOf<R> operator[](size_t index) const {return OriginalRange[Length()-1-index];}

	bool operator==(const RRetro& rhs) const {return OriginalRange==rhs.OriginalRange;}

	template<typename U=R> forceinline Meta::EnableIf<
		HasLength<U>::_,
	size_t> Length() const {return OriginalRange.Length();}

	forceinline RRetro operator()(size_t first, size_t end) const
	{return RRetro(OriginalRange(Length()-end, Length()-first));}

	forceinline const R& Retro() const {return OriginalRange;}
};


template<typename R> forceinline const R& Retro(const RRetro<R>& range) {return range.OriginalRange;}
template<typename R> forceinline R Retro(RRetro<R>&& range) {return Meta::Move(range.OriginalRange);}

template<typename R> forceinline Meta::EnableIf<
	IsAsBidirectionalRange<R>::_,
RRetro<Meta::RemoveConstRef<AsRangeResult<R>>>> Retro(R&& range)
{return {Range::Forward<R>(range)};}

INTRA_WARNING_POP

}}
