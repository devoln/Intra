#pragma once

#include "Intra/Math.h"
#include "Intra/Type.h"
#include "Intra/Range/Concepts.h"
#include "Intra/Range/Operations.h"
#include "Intra/Container/Tuple.h"

INTRA_BEGIN
INTRA_IGNORE_WARN_COPY_IMPLICITLY_DELETED
template<class... Ranges> class Chain
{
	Tuple<Ranges...> mRanges;
	size_t mIndex = 0;
	[[no_unique_address]] TSelect<size_t, EmptyType, (CBidirectionalRange<Ranges> && ...)> mLastIndex;
public:
	static constexpr bool IsAnyInstanceFinite = (CFiniteRange<Ranges> && ...),
		IsAnyInstanceInfinite = (CInfiniteRange<Ranges> || ...);

	template<CAccessibleList... Rs> constexpr Chain(Rs&&... rs): mRanges(ForwardAsRange<Rs>(rs)...) {}

	[[nodiscard]] constexpr decltype(auto) First() const {return ForFieldAtRuntime(mIndex, Intra::First)(mRanges);}
	constexpr void PopFirst()
	{
		INTRA_PRECONDITION(!Empty());
		ForFieldAtRuntime(mIndex, Intra::PopFirst)(mRanges);
		if(ForFieldAtRuntime(mIndex, Intra::IsEmpty)(mRanges)) mIndex++;
	}
	[[nodiscard]] constexpr bool Empty() const noexcept
	{
		if constexpr(CBidirectionalRange<Ranges> && ...) return mIndex > mLastIndex || mIndex == mLastIndex && ForFieldAtRuntime(mIndex, Intra::IsEmpty)(mRanges);
		else return mIndex >= sizeof...(Ranges);
	}

	[[nodiscard]] constexpr decltype(auto) Last() const requires CBidirectionalRange<Ranges> && ... {return ForFieldAtRuntime(mLastIndex, Intra::Last)(mRanges);}
	constexpr void PopLast() requires CBidirectionalRange<Ranges> && ...
	{
		INTRA_PRECONDITION(!Empty());
		ForFieldAtRuntime(mLastIndex, Intra::PopLast)(mRanges);
		if(ForFieldAtRuntime(mLastIndex, Intra::IsEmpty)(mRanges)) mLastIndex--;
	}
	[[nodiscard]] constexpr index_t Length() const requires CHasLength<Ranges> && ...
	{
		auto acc = Accum(Add, Count, 0);
		ForEach(FRef(acc))(mRanges);
		return acc.Result;
	}
	[[nodiscard]] constexpr auto operator[](Index index) const requires (CHasIndex<Ranges> && CHasLength<Ranges>) && ...
	{
		INTRA_PRECONDITION(index < Length());
		const auto getByIndex = [&]<size_t I>(size_t index) {
			auto& range = FieldAt<I>(mRanges);
			if constexpr(I == sizeof...(Ranges) - 1) return range[index];
			else
			{
				auto len = size_t(range.Length());
				if(index < len) return range[index];
				else return getByIndex<I+1>(index - len);
			}
		};
		return getByIndex(index);
	}

	constexpr index_t PopFirstCount(ClampedSize maxElementsToPop) const requires CHasPopFirstCount<Ranges> && ...
	{
		size_t elemsLeftToPop = maxElementsToPop;
		ForEach([&](auto& range) {
			elemsLeftToPop -= range.PopFirstCount(elemsLeftToPop);
		})(mRanges);
		return index_t(maxElementsToPop - elemsLeftToPop);
	}
	constexpr index_t PopLastCount(ClampedSize maxElementsToPop) const requires CHasPopLastCount<Ranges> && ...
	{
		size_t elemsLeftToPop = maxElementsToPop;
		ForEach([&](auto& range) {
			elemsLeftToPop -= range.PopLastCount(elemsLeftToPop);
		})(mRanges);
		return index_t(maxElementsToPop - elemsLeftToPop);
	}
};
template<typename... Rs> Chain(Rs&&...) -> Chain<TRangeOf<Rs&&>...>;
INTRA_END
