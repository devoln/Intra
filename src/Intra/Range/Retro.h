#pragma once

#include "Intra/Range/Concepts.h"


INTRA_BEGIN
template<typename R> struct RRetro
{
	R OriginalRange;

	[[nodiscard]] constexpr bool Empty() const {return OriginalRange.Empty();}
	[[nodiscard]] constexpr decltype(auto) First() const {return OriginalRange.Last();}
	constexpr void PopFirst() {OriginalRange.PopLast();}
	[[nodiscard]] constexpr decltype(auto) Last() const {return OriginalRange.First();}
	constexpr void PopLast() {OriginalRange.PopFirst();}
	
	[[nodiscard]] constexpr decltype(auto) operator[](Index index) const {return OriginalRange[Length()-1-index];}

	template<typename U = R, typename = Requires<
		CHasLength<U>
	>> [[nodiscard]] constexpr auto Length() const {return OriginalRange.Length();}

	[[nodiscard]] constexpr RRetro operator()(size_t first, size_t end) const
	{return RRetro(OriginalRange(Length()-end, Length()-first));}
};


template<typename R> [[nodiscard]] constexpr decltype(auto) Retro(const RRetro<R>& range) {return range.OriginalRange;}
template<typename R> [[nodiscard]] constexpr auto Retro(RRetro<R>&& range) {return Move(range.OriginalRange);}

template<typename R,
	typename AsR = TRangeOfRef<R>
> [[nodiscard]] constexpr Requires<
	CBidirectionalRange<AsR>,
RRetro<TRemoveConstRef<AsR>>> Retro(R&& range)
{return {ForwardAsRange<R>(range)};}
INTRA_END
