#pragma once

#include "Range/Mixins/RangeMixins.h"

namespace Intra { namespace Range {

template<typename R> struct RetroResult:
	RangeMixin<RetroResult<R>, typename R::value_type,
		(R::RangeType>=TypeEnum::Bidirectional)? R::RangeType: TypeEnum::Error,
		true>
{
	typedef typename R::value_type value_type;
	typedef typename R::return_value_type return_value_type;


	R OriginalRange;

	RetroResult(null_t=null): OriginalRange(null) {}
	explicit RetroResult(R&& range): OriginalRange(core::move(range)) {}
	explicit RetroResult(const R& range): OriginalRange(range) {}

	forceinline bool Empty() const {return OriginalRange.Empty();}
	forceinline return_value_type First() const {return OriginalRange.Last();}
	forceinline void PopFirst() {OriginalRange.PopLast();}
	forceinline return_value_type Last() const {return OriginalRange.First();}
	forceinline void PopLast() {OriginalRange.PopFirst();}
	
	template<typename U=R> forceinline Meta::EnableIf<
		IsFiniteRandomAccessRange<U>::_
	> operator[](size_t index) const {return OriginalRange[Length()-1-index];}

	bool operator==(const RetroResult& rhs) const {return OriginalRange==rhs.OriginalRange;}

	template<typename U=R> forceinline Meta::EnableIf<
		HasLength<U>::_,
	size_t> Length() const {return OriginalRange.Length();}

	template<typename U=R> forceinline Meta::EnableIf<
		IsFiniteRandomAccessRange<U>::_,
	RetroResult> opSlice(size_t first, size_t end) const
	{return RetroResult(OriginalRange.opSlice(Length()-end, Length()-first));}

	forceinline const R& Retro() const {return OriginalRange;}
};

template<typename R> forceinline Meta::EnableIf<
	IsBidirectionalRange<R>::_,
RetroResult<Meta::RemoveConstRef<R>>> Retro(R&& range) {return RetroResult<Meta::RemoveConstRef<R>>(core::forward<R>(range));}

}}
