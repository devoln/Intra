#pragma once

#include "Intra/Misc/RawMemory.h"
#include "Intra/Range/Concepts.h"
#include "Intra/Range/Operations.h"
#include "Intra/Range/Decorators.h"

INTRA_BEGIN
INTRA_IGNORE_WARN_SIGN_CONVERSION
INTRA_IGNORE_WARN_LOSING_CONVERSION

/**
  May be faster than WriteTo(src).ConsumeFrom(dst) for short arrays of trivial types
  when their length is not known at compile time.
*/
template<CRange R, class OR, typename P = decltype(Never)>
requires (!CConst<R>) && COutputOf<OR, TRangeValue<R>>
constexpr index_t ReadWriteByOne(R& src, OR& dst)
{
	index_t minLen = 0;
	while(!src.Empty())
	{
		if constexpr(CHasFull<OR> && !CInfiniteRange<OR>)
			if(dst.Full()) break;
		Put(dst, Next(src));
		minLen++;
	}
	return minLen;
}

template<class Dst> struct CopyTo
{
	Dst Dest;
	template<typename R> requires CAsOutputRange<R> || CAssignableList<R>
	constexpr CopyTo(R&& dst) noexcept(noexcept(ForwardAsRange<Dst>(dst))): Dest(ForwardAsRange<Dst>(dst)) {}

	/** Consume the first Min(Count(src), Count(Dest)) of `src` and put them into `Dest`.
	  `Dest` and `src` may overlap only if `src` elements come after `Dest` elements
	  @return The number of copied elements.
	**/
	template<CRange Src> constexpr index_t ConsumeFrom(Src&& src)
	{
		static_assert(COutputOf<Dst, TRangeValue<Src>> || CAssignableRange<Dst>);
		static_assert(!CInfiniteRange<Dst> || !CInfiniteRange<Src>);
		if constexpr(CArrayList<Src> &&
			CTriviallyCopyable<TArrayElement<Src>> &&
			CSame<TArrayElement<Src>, TArrayElement<Dst>>)
		{
			const auto minLen = Min(LengthOf(src), LengthOf(Dest));
			BitwiseCopy(Unsafe, DataOf(Dest), DataOf(src), minLen);
			Dest|PopFirstExactly(minLen);
			src|PopFirstExactly(minLen);
			return minLen;
		}
		else if constexpr(CHasReadWriteMethod<Src&, Dst&>) return src.ReadWrite(Dest);
		else if constexpr(CHasPutAllAdvanceMethod<Dst&, Src&>) return Dest.PutAllAdvance(src);
		else return ReadWriteByOne(src, Dest);
	}

	/** Put the first Min(Count(src), Count(Dest)) of \p src into Dest.
	  `Dest` and `src` may overlap only if `src` elements come after Dest elements
	  @return The number of copied elements.
	*/
	template<CList Src> constexpr index_t From(Src&& src)
	{
		if constexpr(CRange<Src> && CRValueReference<Src&&>) return ConsumeFrom(src);
		else return ConsumeFrom(RangeOf(src));
	}

	template<CList Src> constexpr index_t operator()(Src&& src)
	{
		return From(INTRA_FWD(src));
	}
};
template<typename Dst> CopyTo(Dst&&) -> CopyTo<TRangeOf<Dst&&>>;

template<class Dst> requires COutput<Dst> || CAssignableList<Dst>
[[nodiscard]] constexpr auto WriteTo(Dst& dst) {return CopyTo<Dst&>(dst);}

INTRA_END
