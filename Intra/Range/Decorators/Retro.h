#pragma once

#include "Cpp/Features.h"
#include "Cpp/Warnings.h"
#include "Concepts/Range.h"
#include "Concepts/RangeOf.h"

namespace Intra { namespace Range {

INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS

template<typename R> struct RRetro
{
	R OriginalRange;

	RRetro(null_t=null): OriginalRange(null) {}
	RRetro(R&& range): OriginalRange(Cpp::Move(range)) {}
	RRetro(const R& range): OriginalRange(range) {}

	forceinline bool Empty() const {return OriginalRange.Empty();}
	forceinline Concepts::ReturnValueTypeOf<R> First() const {return OriginalRange.Last();}
	forceinline void PopFirst() {OriginalRange.PopLast();}
	forceinline Concepts::ReturnValueTypeOf<R> Last() const {return OriginalRange.First();}
	forceinline void PopLast() {OriginalRange.PopFirst();}
	
	Concepts::ReturnValueTypeOf<R> operator[](size_t index) const {return OriginalRange[Length()-1-index];}

	bool operator==(const RRetro& rhs) const {return OriginalRange==rhs.OriginalRange;}

	template<typename U=R> forceinline Meta::EnableIf<
		Concepts::HasLength<U>::_,
	size_t> Length() const {return OriginalRange.Length();}

	forceinline RRetro operator()(size_t first, size_t end) const
	{return RRetro(OriginalRange(Length()-end, Length()-first));}

	forceinline const R& Retro() const {return OriginalRange;}
};


template<typename R> forceinline const R& Retro(const RRetro<R>& range) {return range.OriginalRange;}
template<typename R> forceinline R Retro(RRetro<R>&& range) {return Cpp::Move(range.OriginalRange);}

template<typename R,
	typename AsR = Concepts::RangeOfType<R>
> forceinline Meta::EnableIf<
	Concepts::IsBidirectionalRange<AsR>::_,
RRetro<Meta::RemoveConstRef<AsR>>> Retro(R&& range)
{return {Range::Forward<R>(range)};}

INTRA_WARNING_POP

}}
