#pragma once

#include "Core/Functional.h"
#include "Core/Range/Concepts.h"

INTRA_BEGIN
INTRA_WARNING_DISABLE_COPY_MOVE_IMPLICITLY_DELETED
template<typename R, typename F> struct RMap: private F
{
	enum: bool
	{
		RangeIsFinite = CFiniteRange<R>,
		RangeIsInfinite = CInfiniteRange<R>
	};

	constexpr forceinline RMap(R range, F func):
		F(func), OriginalRange(Move(range)) {}

	INTRA_NODISCARD constexpr forceinline decltype(auto) First() const
	{return F::operator()(OriginalRange.First());}

	constexpr forceinline void PopFirst()
	{OriginalRange.PopFirst();}

	constexpr forceinline bool Empty() const
	{return OriginalRange.Empty();}

	constexpr forceinline decltype(auto) Last() const
	{return F::operator()(OriginalRange.Last());}

	constexpr forceinline void PopLast() {OriginalRange.PopLast();}

	constexpr forceinline decltype(auto) operator[](size_t index) const
	{return F::operator()(OriginalRange[index]);}

	template<typename U=R> INTRA_NODISCARD constexpr forceinline Requires<
		CSliceable<U>,
	RMap<TSliceTypeOf<U>, F>> operator()(size_t start, size_t end) const
	{return {OriginalRange(start, end), *this};}

	template<typename U = R> INTRA_NODISCARD constexpr forceinline Requires<
		CHasLength<U>,
	index_t> Length() const {return OriginalRange.Length();}

	R OriginalRange;
};

template<typename R, typename F,
	typename AsR = TRangeOfType<R>,
	typename T = TReturnValueTypeOf<AsR>,
	typename AsF = TFunctorOf<F>
> INTRA_NODISCARD constexpr forceinline Requires<
	CConsumableRange<AsR> &&
	CCallable<AsF, T>,
RMap<TRemoveConstRef<AsR>, TRemoveReference<AsF>>> Map(R&& range, F&& func)
{return {ForwardAsRange<R>(range), ForwardAsFunc<F>(func)};}
INTRA_END
