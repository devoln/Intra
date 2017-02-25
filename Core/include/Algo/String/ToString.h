#pragma once

#include "Meta/Type.h"
#include "Meta/EachField.h"
#include "Range/Concepts.h"
#include "Range/Operations.h"
#include "Range/ForwardDecls.h"
#include "Range/Generators/Count.h"
#include "Range/Generators/Repeat.h"
#include "Range/Generators/StringView.h"
#include "Algo/Op.h"
#include "Math/Math.h"
#include "Algo/Mutation/Copy.h"
#include "ToStringArithmetic.h"
#include "Range/AsRange.h"

namespace Intra { namespace Algo {

using namespace Range::Concepts;

INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS

/*template<typename R, typename Char, size_t N> forceinline Meta::EnableIf<
	IsOutputCharRange<R>::_ &&
	Meta::IsCharType<Char>::_,
R&&> operator<<(R&& dst, const Char(&str)[N])
{
	CopyToAdvance(str, dst);
	return Meta::Forward<R>(dst);
}*/

template<typename R, typename Char> forceinline Meta::EnableIf<
	IsOutputCharRange<R>::_ &&
	Meta::IsCharType<Char>::_,
R&&> operator<<(R&& dst, const Char* str)
{
	ToString(dst, str);
	return Meta::Forward<R>(dst);
}

template<typename R, typename X> forceinline Meta::EnableIf<
	IsOutputCharRange<R>::_ &&
	!Meta::IsCharType<X>::_ &&
	Meta::IsArithmeticType<X>::_,
R&&> operator<<(R&& dst, X number)
{
	ToString(dst, number);
	return Meta::Forward<R>(dst);
}

template<typename R, typename Char> forceinline Meta::EnableIf<
	IsOutputCharRange<R>::_ &&
	Meta::IsCharType<Char>::_,
R&&> operator<<(R&& dst, Char ch)
{
	dst.Put(ch);
	return Meta::Forward<R>(dst);
}

template<typename R, typename SRC> forceinline Meta::EnableIf<
	IsOutputCharRange<R>::_ &&
	IsAsConsumableRange<SRC>::_ &&
	Meta::IsCharType<ValueTypeOfAs<SRC>>::_,
R&&> operator<<(R&& dst, SRC&& src)
{
	CopyToAdvance(Range::Forward<SRC>(src), dst);
	return Meta::Forward<R>(dst);
}


namespace D {

template<typename Range, typename CR> struct TupleAppender
{
	bool First;
	Range& DstRange;
	const CR& Separator;

	TupleAppender(bool first, Range& dstRange, const CR& elementSeparator):
		First(first), DstRange(dstRange), Separator(elementSeparator) {}

	TupleAppender& operator=(const TupleAppender&) = delete;

	template<typename V> void operator()(const V& value);
};

}

template<typename R, typename Tuple> Meta::EnableIf<
	IsOutputCharRange<R>::_ &&
	Meta::HasForEachField<Tuple>::_ &&
	!IsAsConsumableRange<Tuple>::_,
R&&> operator<<(R&& dst, Tuple&& tuple)
{
	dst.Put('[');
	StringView sep = ", ";
	D::TupleAppender<R, StringView> appender(true, dst, sep);
	Meta::ForEachField(Meta::Forward<Tuple>(tuple), appender);
	dst.Put(']');
	return Meta::Forward<R>(dst);
}

template<typename R, typename Tuple, typename SR, typename LR, typename RR> Meta::EnableIf<
	IsOutputCharRange<R>::_ &&
	Meta::HasForEachField<Tuple>::_ &&
	!IsAsConsumableRange<Tuple>::_ &&
	IsAsCharRange<SR>::_ && IsAsCharRange<LR>::_ && IsAsCharRange<RR>::_
> ToString(R&& dst, Tuple&& tuple, SR&& separator, LR&& lBracket, RR&& rBracket)
{
	CopyToAdvance(Range::Forward<LR>(lBracket), dst);
	auto sep = Range::Forward<SR>(separator);
	D::TupleAppender<R, AsRangeResultNoCRef<SR>> appender(true, dst, sep);
	Meta::ForEachField(Meta::Forward<Tuple>(tuple), appender);
	CopyToAdvance(Range::Forward<RR>(rBracket), dst);
}

template<typename R, typename VR, typename SR, typename LR, typename RR> Meta::EnableIf<
	IsOutputCharRange<R>::_ &&
	IsAsConsumableRange<VR>::_ && !Meta::IsCharType<ValueTypeOfAs<VR>>::_ &&
	IsAsForwardCharRange<SR>::_ &&
	IsAsCharRange<LR>::_ && IsAsCharRange<RR>::_
> ToString(R&& dst, VR&& r, SR&& separator, LR&& lBracket, RR&& rBracket)
{
	auto range = Range::Forward<VR>(r);
	CopyToAdvance(Range::Forward<LR>(lBracket), dst);
	if(!range.Empty())
	{
		dst << range.First();
		range.PopFirst();
	}
	while(!range.Empty())
	{
		CopyToAdvance(separator, dst);
		dst << range.First();
		range.PopFirst();
	}
	CopyToAdvance(Range::Forward<RR>(rBracket), dst);
}

template<typename R, typename VR> Meta::EnableIf<
	IsOutputCharRange<R>::_ &&
	IsAsConsumableRange<VR>::_ &&
	!Meta::IsCharType<ValueTypeOfAs<VR>>::_,
R&&> operator<<(R&& dst, VR&& r)
{
	ToString(dst, Range::Forward<VR>(r),
		StringView(", "), StringView("["), StringView("]"));
	return Meta::Forward<R>(dst);
}

namespace D {

template<typename Range, typename CR> template<typename V>
void TupleAppender<Range, CR>::operator()(const V& value)
{
	if(!First) CopyToAdvance(Separator, DstRange);
	DstRange << value;
	First = false;
}

}

template<typename OR, typename T> forceinline Meta::EnableIf<
	IsOutputCharRange<OR>::_ &&
	!Meta::IsArithmeticType<T>::_
> ToString(OR&& dst, T&& v) {dst << Meta::Forward<T>(v);}

INTRA_WARNING_POP

}}
