#pragma once

#include "Intra/Operations.h"
#include "Intra/Range/Concepts.h"
#include "Intra/Range/Count.h"
#include "Intra/Range/Take.h"
#include "Intra/Range/Operations.h"

INTRA_BEGIN
INTRA_IGNORE_WARNING_ASSIGN_IMPLICITLY_DELETED
template<typename R> struct RCycle
{
	static_assert(CForwardRange<R> && !CInfiniteInputRange<R>);
	static constexpr bool IsAnyInstanceInfinite = true;

	template<typename U = R, typename = void, typename = Requires<!CRandomAccessRange<R>>> constexpr RCycle(R range):
		mOriginalRange(Move(range)), mOffsetRange(mOriginalRange) {INTRA_PRECONDITION(!mOriginalRange.Empty());}

	template<typename U = R, typename = Requires<CRandomAccessRange<U>>> constexpr RCycle(R range):
		mOriginalRange(Move(range)) {INTRA_PRECONDITION(!mOriginalRange.Empty());}

	[[nodiscard]] constexpr bool Empty() const {return false;}

	[[nodiscard]] constexpr const decltype(auto) First() const
	{
		if constexpr(CRandomAccessRange<R>) return mOriginalRange[mOffsetRange.Counter];
		else return mOffsetRange.First();
	}

	constexpr void PopFirst()
	{
		mOffsetRange.PopFirst();
		if constexpr(CRandomAccessRange<R>)
		{
			if constexpr(CHasLength<R>)
				if(mOffsetRange.Counter == size_t(mOriginalRange.Length())) mOffsetRange.Counter = 0;
		}
		else if(mOffsetRange.Empty()) mOffsetRange = mOriginalRange;
	}

	template<typename = Requires<CRandomAccessRange<R> || CHasPopFirstCount<R> || CHasLength<R>>>
	constexpr index_t PopFirstCount(ClampedSize elementsToPop) const
	{
		if constexpr(CRandomAccessRange<R>)
		{
			mOffsetRange.Counter += size_t(elementsToPop);
			mOffsetRange.Counter %= size_t(mOriginalRange.Length());
		}
		else
		{
			auto leftToPop = size_t(elementsToPop);
			if constexpr(CHasLength<R>)
			{
				leftToPop %= mOriginalRange.Length();
				const auto offLen = size_t(mOffsetRange.Length());
				if(offLen < leftToPop)
				{
					mOffsetRange = mOriginalRange;
					leftToPop -= offLen;
				}
				mOffsetRange|PopFirstExactly(leftToPop);
			}
			else while(leftToPop)
			{
				leftToPop -= mOffsetRange|Intra::PopFirstCount(leftToPop);
				if(mOffsetRange.Empty()) mOffsetRange = mOriginalRange;
			}
		}
		return index_t(elementsToPop);
	}

	template<typename = Requires<CRandomAccessRange<R>>>
	[[nodiscard]] constexpr decltype(auto) operator[](Index index) const
	{
		size_t i = mOffsetRange.Counter + size_t(index);
		i %= size_t(mOriginalRange.Length());
		return mOriginalRange[i];
	}

private:
	R mOriginalRange;
	TSelect<RCount<int, size_t>, R, CRandomAccessRange<R>> mOffsetRange;
};
template<class R> RCycle(R) -> RCycle<R>;

template<typename R, typename = Requires<
	CAsForwardRange<R> ||
	CAsInfiniteInputRange<R> && CAsConsumableRange<R>
>> [[nodiscard]] constexpr auto Cycle(R&& range)
{
	if constexpr(CAsInfiniteInputRange<R>) return ForwardAsRange<R>(range);
	else return RCycle(ForwardAsRange<R>(range));
}
INTRA_END
