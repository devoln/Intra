#pragma once

#include "Core/Range/Concepts.h"


INTRA_CORE_RANGE_BEGIN
template<typename R> struct RRetro
{
	R OriginalRange;

	INTRA_NODISCARD constexpr forceinline bool Empty() const {return OriginalRange.Empty();}
	INTRA_NODISCARD constexpr forceinline decltype(auto) First() const {return OriginalRange.Last();}
	INTRA_CONSTEXPR2 forceinline void PopFirst() {OriginalRange.PopLast();}
	INTRA_NODISCARD constexpr forceinline decltype(auto) Last() const {return OriginalRange.First();}
	INTRA_CONSTEXPR2 forceinline void PopLast() {OriginalRange.PopFirst();}
	
	INTRA_NODISCARD constexpr decltype(auto) operator[](size_t index) const {return OriginalRange[Length()-1-index];}

	template<typename U = R> INTRA_NODISCARD constexpr forceinline Requires<
		CHasLength<U>,
	index_t> Length() const {return OriginalRange.Length();}

	INTRA_NODISCARD constexpr forceinline RRetro operator()(size_t first, size_t end) const
	{return RRetro(OriginalRange(Length()-end, Length()-first));}
};


template<typename R> INTRA_NODISCARD constexpr forceinline const R& Retro(const RRetro<R>& range) {return range.OriginalRange;}
template<typename R> INTRA_NODISCARD constexpr forceinline R Retro(RRetro<R>&& range) {return Move(range.OriginalRange);}

template<typename R,
	typename AsR = TRangeOfType<R>
> INTRA_NODISCARD constexpr forceinline Requires<
	CBidirectionalRange<AsR>,
RRetro<TRemoveConstRef<AsR>>> Retro(R&& range)
{return {ForwardAsRange<R>(range)};}
INTRA_CORE_RANGE_END
