#pragma once

#include "Intra/Assert.h"
#include "Intra/Range/Concepts.h"
#include "Intra/Range/Operations.h"

INTRA_BEGIN
INTRA_IGNORE_WARNING_ASSIGN_IMPLICITLY_DELETED
template<typename R> struct RStride
{
	static constexpr bool IsAnyInstanceFinite = CFiniteRange<R>,
		IsAnyInstanceInfinite = CInfiniteRange<R>;

	RStride() = default;

	constexpr RStride(R range, index_t strideStep):
		mOriginalRange(Move(range)), mStep(strideStep)
	{
		INTRA_PRECONDITION(strideStep > 0);
		skip_back_odd();
	}

	[[nodiscard]] constexpr bool Empty() const {return mOriginalRange.Empty();}
	[[nodiscard]] constexpr decltype(auto) First() const {return mOriginalRange.First();}
	constexpr void PopFirst() {PopFirstCount(mOriginalRange, mStep);}

	template<typename U = R, typename = Requires<
		CHasLast<U>
	>> [[nodiscard]] constexpr decltype(auto) Last() const {return mOriginalRange.Last();}

	template<typename U = R> constexpr Requires<
		CHasPopLast<U>
	> PopLast() {PopLastCount(mOriginalRange, mStep);}

	template<typename U = R, typename = Requires<CHasIndex<U>>>
	[[nodiscard]] constexpr decltype(auto) operator[](Index index) const {return mOriginalRange[index*mStep];}

	[[nodiscard]] constexpr index_t PopFirstCount(ClampedSize maxElementsToPop)
	{
		return index_t((size_t(PopFirstCount(mOriginalRange, mStep*size_t(maxElementsToPop))) + mStep - 1) / mStep);
	}

	template<typename U = R> [[nodiscard]] constexpr Requires<
		CHasPopLast<U>,
	index_t> PopLastCount(ClampedSize maxElementsToPop)
	{
		return index_t((size_t(PopLastCount(mOriginalRange, mStep*size_t(maxElementsToPop))) + mStep - 1) / mStep);
	}

	[[nodiscard]] constexpr index_t Length() const {return index_t((size_t(mOriginalRange.Length()) + mStep - 1) / mStep);}

	template<typename R> [[nodiscard]] friend constexpr RStride<R> Stride(const RStride<R>& range, Size step) const
	{
		return {mOriginalRange, mStep*size_t(step)};
	}

private:
	template<typename U = R, typename = Requires<
		CHasPopLast<U> &&
		CHasLength<U>
	>> constexpr void skip_back_odd()
	{
		const size_t len = size_t(mOriginalRange.Length());
		if(len == 0) return;
		PopLastExactly(mOriginalRange, (len-1) % mStep);
	}

	template<typename U = R> constexpr Requires<
		!(CHasPopLast<U> && CHasLength<U>)
	> skip_back_odd() {}

	R mOriginalRange;
	size_t mStep = 0;
};

template<typename R, typename AsR = TRangeOf<R>> [[nodiscard]] constexpr Requires<
	CAccessibleRange<AsR>,
RStride<AsR>> Stride(R&& range, Size step)
{
	return {ForwardAsRange<R>(range), size_t(step)};
}
INTRA_END
