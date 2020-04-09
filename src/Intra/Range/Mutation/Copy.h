#pragma once

#include "Intra/Misc/RawMemory.h"
#include "Intra/Range/Concepts.h"
#include "Intra/Range/Operations.h"
#include "Intra/Range/Take.h"

INTRA_BEGIN
INTRA_IGNORE_WARNING_SIGN_CONVERSION
INTRA_IGNORE_WARNING_LOSING_CONVERSION

/**
  May be faster than WriteTo(src).ConsumeFrom(dst) for short arrays of trivial types where their length is not known at compile time.
*/
template<class R, class OR, typename P = TNot<TAlways>> constexpr Requires<
	CInputRange<R> && !CConst<R> &&
	COutputRangeOf<OR, TValueTypeOf<R>>,
index_t> ReadWriteByOne(R& src, OR& dst)
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
	template<typename R, typename = Requires<CAsOutputRange<R> || CAsAssignableRange<R>>>
	constexpr CopyTo(R&& dst) noexcept(ForwardAsRange<Dst>(dst)): Dest(ForwardAsRange<Dst>(dst)) {}

	/** Consume the first Min(Count(src), Count(Dest)) of \p src and put them into Dest.
	  Dest and `\p src may overlap only if \p src elements come after Dest elements
	  @return The number of copied elements.
	**/
	template<class Src> constexpr Requires<
		CInputRange<Src>,
	index_t> ConsumeFrom(Src&& src)
	{
		static_assert(COutputRangeOf<Dst, TValueTypeOf<Src>> || CAssignableRange<Dst>);
		static_assert(!CInfiniteInputRange<Dst> || !CInfiniteInputRange<Src>);
		if constexpr(CTrivCopyCompatibleArrayWith<Src, Dst>)
		{
			const auto minLen = FMin(LengthOf(src), LengthOf(dst));
			Misc::BitwiseCopyUnsafe(DataOf(Dest), DataOf(src), minLen);
			Dest|PopFirstExactly(minLen);
			src|PopFirstExactly(minLen);
			return minLen;
		}
		else if constexpr(CHasReadWriteMethod<Src&, Dst&>) return src.ReadWrite(Dest);
		else if constexpr(CHasPutAllAdvanceMethod<Dst&, Src&>) return Dest.PutAllAdvance(src);
		return ReadWriteByOne(src, Dest);
	}

	/** Put the first Min(Count(src), Count(Dest)) of \p src into Dest.
	  Dest and `\p src may overlap only if \p src elements come after Dest elements
	  @return The number of copied elements.
	*/
	template<typename Src> constexpr Requires<
		CAsInputRange<Src>,
	index_t> From(Src&& src)
	{
		if constexpr(CInputRange<Src> && CRValueReference<Src&&>) return ConsumeFrom(src);
		else return ConsumeFrom(RangeOf(src));
	}

	template<typename Src> constexpr Requires<
		CAsInputRange<Src>,
	index_t> operator()(Src&& src)
	{
		return From(Forward<Src>(src));
	}
};
template<typename Dst> CopyTo(Dst&&) -> CopyTo<TRangeOf<Dst&&>>;

template<class Dst, typename = Requires<CInputRange<Dst>>>
[[nodiscard]] constexpr auto WriteTo(Dst& dst) {return CopyTo<Dst&>(dst);}

INTRA_END
