#pragma once

#include "Meta/Type.h"
#include "Meta/EachField.h"
#include "Range/Concepts.h"
#include "Range/Operations.h"
#include "Range/ForwardDecls.h"
#include "Range/Generators/Count.h"
#include "Range/Generators/Repeat.h"
#include "Algo/Op.h"
#include "Math/Math.h"
#include "Algo/Mutation/Copy.h"
#include "ToStringArithmetic.h"
#include "Range/AsRange.h"

namespace Intra { namespace Algo {

INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS

/*template<typename R, typename Char, size_t N> forceinline Meta::EnableIf<
	Range::IsOutputCharRange<R>::_ &&
	Meta::IsCharType<Char>::_,
R&&> operator<<(R&& dst, const Char(&str)[N])
{
	Algo::CopyToAdvance(str, dst);
	return Meta::Forward<R>(dst);
}*/

template<typename R, typename Char> forceinline Meta::EnableIf<
	Range::IsOutputCharRange<R>::_ &&
	Meta::IsCharType<Char>::_,
R&&> operator<<(R&& dst, const Char* str)
{
	Algo::ToString(dst, str);
	return Meta::Forward<R>(dst);
}

template<typename R, typename X> forceinline Meta::EnableIf<
	Range::IsOutputCharRange<R>::_ &&
	!Meta::IsCharType<X>::_ &&
	Meta::IsArithmeticType<X>::_,
R&&> operator<<(R&& dst, X number)
{
	ToString(dst, number);
	return Meta::Forward<R>(dst);
}

template<typename R, typename Char> forceinline Meta::EnableIf<
	Range::IsOutputCharRange<R>::_ &&
	Meta::IsCharType<Char>::_,
R&&> operator<<(R&& dst, Char ch)
{
	dst.Put(ch);
	return Meta::Forward<R>(dst);
}

template<typename R, typename SRC> forceinline Meta::EnableIf<
	Range::IsOutputCharRange<R>::_ &&
	Range::IsAsConsumableRange<SRC>::_ &&
	Meta::IsCharType<Range::ValueTypeOfAs<SRC>>::_,
R&&> operator<<(R&& dst, SRC&& src)
{
	Algo::CopyToAdvance(Range::Forward<SRC>(src), dst);
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
	Range::IsOutputCharRange<R>::_ &&
	Meta::HasForEachField<Tuple>::_ &&
	!Range::IsAsConsumableRange<Tuple>::_,
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
	Range::IsOutputCharRange<R>::_ &&
	Meta::HasForEachField<Tuple>::_ &&
	!Range::IsAsConsumableRange<Tuple>::_ &&
	Range::IsAsCharRange<SR>::_ && Range::IsAsCharRange<LR>::_ && Range::IsAsCharRange<RR>::_
> ToString(R&& dst, Tuple&& tuple, SR&& separator, LR&& lBracket, RR&& rBracket)
{
	Algo::CopyToAdvance(Range::Forward<LR>(lBracket), dst);
	auto sep = Range::Forward<SR>(separator);
	D::TupleAppender<R, Meta::RemoveConstRef<Range::AsRangeResult<SR>>> appender(true, dst, sep);
	Meta::ForEachField(Meta::Forward<Tuple>(tuple), appender);
	Algo::CopyToAdvance(Range::Forward<RR>(rBracket), dst);
}

template<typename R, typename VR, typename SR, typename LR, typename RR> Meta::EnableIf<
	Range::IsOutputCharRange<R>::_ &&
	Range::IsAsConsumableRange<VR>::_ && !Meta::IsCharType<Range::ValueTypeOfAs<VR>>::_ &&
	Range::IsAsForwardCharRange<SR>::_ &&
	Range::IsAsCharRange<LR>::_ && Range::IsAsCharRange<RR>::_
> ToString(R&& dst, VR&& r, SR&& separator, LR&& lBracket, RR&& rBracket)
{
	auto range = Range::Forward<VR>(r);
	Algo::CopyToAdvance(Range::Forward<LR>(lBracket), dst);
	if(!range.Empty())
	{
		dst << range.First();
		range.PopFirst();
	}
	while(!range.Empty())
	{
		Algo::CopyToAdvance(separator, dst);
		dst << range.First();
		range.PopFirst();
	}
	Algo::CopyToAdvance(Range::Forward<RR>(rBracket), dst);
}

template<typename R, typename VR> Meta::EnableIf<
	Range::IsOutputCharRange<R>::_ &&
	Range::IsAsConsumableRange<VR>::_ &&
	!Meta::IsCharType<Range::ValueTypeOfAs<VR>>::_,
R&&> operator<<(R&& dst, VR&& r)
{
	Algo::ToString(dst, Range::Forward<VR>(r),
		Range::AsRange({',', ' '}), Range::AsRange({'['}), Range::AsRange({']'}));
	return Meta::Forward<R>(dst);
}

namespace D {

template<typename Range, typename CR> template<typename V>
void TupleAppender<Range, CR>::operator()(const V& value)
{
	if(!First) Algo::CopyToAdvance(Separator, DstRange);
	DstRange << value;
	First = false;
}

}

template<typename OR, typename T> forceinline Meta::EnableIf<
	Range::IsOutputCharRange<OR>::_ &&
	!Meta::IsArithmeticType<T>::_
> ToString(OR&& dst, T&& v) {dst << Meta::Forward<T>(v);}

INTRA_WARNING_POP

}}
