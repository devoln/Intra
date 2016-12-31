#pragma once

#include "Range/ForwardDecls.h"
#include "Range/Concepts.h"

namespace Intra { namespace Range {

template<typename R> struct RRetro
{
	R OriginalRange;

	RRetro(null_t=null): OriginalRange(null) {}
	explicit RRetro(R&& range): OriginalRange(Meta::Move(range)) {}
	explicit RRetro(const R& range): OriginalRange(range) {}

	forceinline bool Empty() const {return OriginalRange.Empty();}
	forceinline ReturnValueTypeOf<R> First() const {return OriginalRange.Last();}
	forceinline void PopFirst() {OriginalRange.PopLast();}
	forceinline ReturnValueTypeOf<R> Last() const {return OriginalRange.First();}
	forceinline void PopLast() {OriginalRange.PopFirst();}
	
	template<typename U=R> forceinline Meta::EnableIf<
		HasIndex<U>::_
	> operator[](size_t index) const {return OriginalRange[Length()-1-index];}

	bool operator==(const RRetro& rhs) const {return OriginalRange==rhs.OriginalRange;}

	template<typename U=R> forceinline Meta::EnableIf<
		HasLength<U>::_,
	size_t> Length() const {return OriginalRange.Length();}

	template<typename U=R> forceinline Meta::EnableIf<
		HasSlicing<U>::_,
	RRetro> operator()(size_t first, size_t end) const
	{return RRetro(OriginalRange(Length()-end, Length()-first));}

	forceinline const R& Retro() const {return OriginalRange;}
};


template<typename R> forceinline R Retro(const RRetro<R>& range) {return range.OriginalRange;}
template<typename R> forceinline R Retro(RRetro<R>&& range) {return Meta::Move(range.OriginalRange);}

template<typename R> forceinline Meta::EnableIf<
	IsBidirectionalRange<R>::_,
RRetro<Meta::RemoveConstRef<R>>> Retro(R&& range)
{return RRetro<Meta::RemoveConstRef<R>>(Meta::Forward<R>(range));}

template<typename T, size_t N> RRetro<AsRangeResult<T(&)[N]>> Retro(T(&arr)[N])
{return Retro(AsRange(arr));}

}}
