#pragma once

#include "Intra/EachField.h"
#include "Intra/Range/Concepts.h"
#include "Intra/Range/StringView.h"
#include "Intra/Range/Operations.h"

#include "Intra/Range/Count.h"
#include "Intra/Range/Repeat.h"
#include "Intra/Range/Mutation/Copy.h"
#include "ToStringArithmetic.h"


INTRA_BEGIN
/*template<typename R, typename Char, size_t N> INTRA_FORCEINLINE Requires<
	IsOutputCharRange<R>::_ &&
	CChar<Char>::_,
R&&> operator<<(R&& dst, const Char(&str)[N])
{
	WriteTo(str, dst);
	return Forward<R>(dst);
}*/

template<typename R, typename Char> constexpr Requires<
	COutputCharRange<R> &&
	CChar<Char>,
R&&> operator<<(R&& dst, const Char* str)
{
	ToString(dst, str);
	return Forward<R>(dst);
}

template<typename R, typename X> constexpr Requires<
	COutputCharRange<R> &&
	!CChar<X> &&
	CArithmetic<X>,
R&&> operator<<(R&& dst, X number)
{
	ToString(dst, number);
	return Forward<R>(dst);
}

template<typename R, typename Char> constexpr Requires<
	COutputCharRange<R> &&
	CChar<Char>,
R&&> operator<<(R&& dst, Char ch)
{
	dst.Put(ch);
	return Forward<R>(dst);
}

template<typename R, typename SRC,
	typename AsSRC = TRangeOfRef<SRC>
> constexpr Requires<
	COutputCharRange<R> &&
	CConsumableRange<AsSRC> &&
	CChar<TValueTypeOf<AsSRC>>,
R&&> operator<<(R&& dst, SRC&& src)
{
	WriteTo(ForwardAsRange<SRC>(src), dst);
	return Forward<R>(dst);
}


namespace z_D {

template<typename Range, typename CR> struct TupleAppender
{
	bool First;
	Range& DstRange;
	const CR& Separator;

	constexpr TupleAppender(bool first, Range& dstRange, const CR& elementSeparator):
		First(first), DstRange(dstRange), Separator(elementSeparator) {}

	TupleAppender& operator=(const TupleAppender&) = delete;

	template<typename V> void operator()(const V& value);
};

}

template<typename R, typename Tuple> constexpr Requires<
	COutputCharRange<R> &&
	CHasForEachField<Tuple> &&
	!CAsConsumableRange<Tuple>,
R&&> operator<<(R&& dst, Tuple&& tuple)
{
	dst.Put('[');
	StringView sep = ", ";
	z_D::TupleAppender<R, StringView> appender(true, dst, sep);
	ForEachField(Forward<Tuple>(tuple), appender);
	dst.Put(']');
	return Forward<R>(dst);
}

template<typename R, typename Tuple, typename SR, typename LR, typename RR> Requires<
	COutputCharRange<R> &&
	CHasForEachField<Tuple> &&
	!CAsConsumableRange<Tuple> &&
	CAsCharRange<SR> &&
	CAsCharRange<LR> &&
	CAsCharRange<RR>
> ToString(R&& dst, Tuple&& tuple, SR&& separator, LR&& lBracket, RR&& rBracket)
{
	WriteTo(ForwardAsRange<LR>(lBracket), dst);
	auto sep = ForwardAsRange<SR>(separator);
	z_D::TupleAppender<R, TRangeOf<SR>> appender(true, dst, sep);
	ForEachField(Forward<Tuple>(tuple), appender);
	WriteTo(ForwardAsRange<RR>(rBracket), dst);
}

template<typename R, typename VR, typename SR, typename LR, typename RR> Requires<
	COutputCharRange<R> &&
	CAsConsumableRange<VR> &&
	!CChar<TValueTypeOfAs<VR>> &&
	CAsForwardCharRange<SR> &&
	CAsCharRange<LR> &&
	CAsCharRange<RR>
> ToString(R&& dst, VR&& r, SR&& separator, LR&& lBracket, RR&& rBracket)
{
	auto range = ForwardAsRange<VR>(r);
	WriteTo(ForwardAsRange<LR>(lBracket), dst);
	if(!range.Empty())
	{
		dst << range.First();
		range.PopFirst();
	}
	while(!range.Empty())
	{
		WriteTo(separator, dst);
		dst << range.First();
		range.PopFirst();
	}
	WriteTo(ForwardAsRange<RR>(rBracket), dst);
}

template<typename R, typename VR> Requires<
	COutputCharRange<R> &&
	CAsConsumableRange<VR> &&
	!CChar<TValueTypeOfAs<VR>>,
R&&> operator<<(R&& dst, VR&& r)
{
	ToString(dst, ForwardAsRange<VR>(r),
		StringView(", "), StringView("["), StringView("]"));
	return Forward<R>(dst);
}

namespace z_D {

template<typename Range, typename CR> template<typename V>
void TupleAppender<Range, CR>::operator()(const V& value)
{
	if(!First) WriteTo(Separator, DstRange);
	DstRange << value;
	First = false;
}

}

template<typename OR, typename T> INTRA_FORCEINLINE Requires<
	COutputCharRange<OR> &&
	!CArithmetic<TRemoveReference<T>>
> ToString(OR&& dst, T&& v) {dst << Forward<T>(v);}
INTRA_END
