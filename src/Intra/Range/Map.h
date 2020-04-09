#pragma once

#include "Intra/TypeSafe.h"
#include "Intra/Functional.h"
#include "Intra/Range/Concepts.h"
#include "Intra/Range/Operations.h"

INTRA_BEGIN
INTRA_IGNORE_WARNING_COPY_MOVE_IMPLICITLY_DELETED
template<typename R, typename F> struct RMap: private F
{
	static constexpr bool
		IsAnyInstanceFinite = CFiniteRange<R>,
		IsAnyInstanceInfinite = CInfiniteRange<R>;

	constexpr RMap(R range, F func):
		F(func), OriginalRange(Move(range)) {}

	[[nodiscard]] constexpr decltype(auto) First() const
	{return F::operator()(OriginalRange.First());}

	constexpr void PopFirst()
	{OriginalRange.PopFirst();}

	constexpr bool Empty() const
	{return OriginalRange.Empty();}

	constexpr decltype(auto) Last() const
	{return F::operator()(OriginalRange.Last());}

	constexpr void PopLast() {OriginalRange.PopLast();}

	constexpr decltype(auto) operator[](size_t index) const
	{return F::operator()(OriginalRange[index]);}

	[[nodiscard]] constexpr auto PopFirstCount(ClampedSize numElementsToPop)
	{return ::Intra::PopFirstCount(*this, numElementsToPop);}

	template<typename U = R, typename = Requires<CBidirectionalRange<U>>>
	[[nodiscard]] constexpr auto PopLastCount(ClampedSize numElementsToPop)
	{return ::Intra::PopLastCount(*this, numElementsToPop);}

	template<typename U = R> [[nodiscard]] constexpr Requires<
		CHasLength<U>,
	index_t> Length() const {return OriginalRange.Length();}

	R OriginalRange;
};

template<typename R, typename F,
	typename AsR = TRangeOfRef<R>,
	typename T = TReturnValueTypeOf<AsR>,
	typename AsF = TFunctorOf<F>
> [[nodiscard]] constexpr Requires<
	CConsumableRange<AsR> &&
	CCallable<AsF, T>,
RMap<TRemoveConstRef<AsR>, TRemoveReference<AsF>>> Map(R&& range, F&& func)
{return {ForwardAsRange<R>(range), ForwardAsFunc<F>(func)};}
INTRA_END
