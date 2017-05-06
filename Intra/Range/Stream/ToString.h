#pragma once

#include "Meta/Type.h"
#include "Meta/EachField.h"

#include "Concepts/Range.h"
#include "Concepts/RangeOf.h"

#include "Utils/StringView.h"
#include "Utils/Op.h"

#include "Range/Operations.h"
#include "Range/ForwardDecls.h"
#include "Range/Generators/Count.h"
#include "Range/Generators/Repeat.h"
#include "Range/Mutation/Copy.h"
#include "ToStringArithmetic.h"


namespace Intra { namespace Range {

INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS

/*template<typename R, typename Char, size_t N> forceinline Meta::EnableIf<
	IsOutputCharRange<R>::_ &&
	Meta::IsCharType<Char>::_,
R&&> operator<<(R&& dst, const Char(&str)[N])
{
	CopyToAdvance(str, dst);
	return Cpp::Forward<R>(dst);
}*/

template<typename R, typename Char> forceinline Meta::EnableIf<
	Concepts::IsOutputCharRange<R>::_ &&
	Meta::IsCharType<Char>::_,
R&&> operator<<(R&& dst, const Char* str)
{
	ToString(dst, str);
	return Cpp::Forward<R>(dst);
}

template<typename R, typename X> forceinline Meta::EnableIf<
	Concepts::IsOutputCharRange<R>::_ &&
	!Meta::IsCharType<X>::_ &&
	Meta::IsArithmeticType<X>::_,
R&&> operator<<(R&& dst, X number)
{
	ToString(dst, number);
	return Cpp::Forward<R>(dst);
}

template<typename R, typename Char> forceinline Meta::EnableIf<
	Concepts::IsOutputCharRange<R>::_ &&
	Meta::IsCharType<Char>::_,
R&&> operator<<(R&& dst, Char ch)
{
	dst.Put(ch);
	return Cpp::Forward<R>(dst);
}

template<typename R, typename SRC,
	typename AsSRC = Concepts::RangeOfType<SRC>
> forceinline Meta::EnableIf<
	Concepts::IsOutputCharRange<R>::_ &&
	Concepts::IsConsumableRange<AsSRC>::_ &&
	Meta::IsCharType<Concepts::ValueTypeOf<AsSRC>>::_,
R&&> operator<<(R&& dst, SRC&& src)
{
	CopyToAdvance(Range::Forward<SRC>(src), dst);
	return Cpp::Forward<R>(dst);
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
	Concepts::IsOutputCharRange<R>::_ &&
	Meta::HasForEachField<Tuple>::_ &&
	!Concepts::IsAsConsumableRange<Tuple>::_,
R&&> operator<<(R&& dst, Tuple&& tuple)
{
	dst.Put('[');
	StringView sep = ", ";
	D::TupleAppender<R, StringView> appender(true, dst, sep);
	Meta::ForEachField(Cpp::Forward<Tuple>(tuple), appender);
	dst.Put(']');
	return Cpp::Forward<R>(dst);
}

template<typename R, typename Tuple, typename SR, typename LR, typename RR> Meta::EnableIf<
	Concepts::IsOutputCharRange<R>::_ &&
	Meta::HasForEachField<Tuple>::_ &&
	!Concepts::IsAsConsumableRange<Tuple>::_ &&
	Concepts::IsAsCharRange<SR>::_ &&
	Concepts::IsAsCharRange<LR>::_ &&
	Concepts::IsAsCharRange<RR>::_
> ToString(R&& dst, Tuple&& tuple, SR&& separator, LR&& lBracket, RR&& rBracket)
{
	CopyToAdvance(Range::Forward<LR>(lBracket), dst);
	auto sep = Range::Forward<SR>(separator);
	D::TupleAppender<R, Concepts::RangeOfTypeNoCRef<SR>> appender(true, dst, sep);
	Meta::ForEachField(Cpp::Forward<Tuple>(tuple), appender);
	CopyToAdvance(Range::Forward<RR>(rBracket), dst);
}

template<typename R, typename VR, typename SR, typename LR, typename RR> Meta::EnableIf<
	Concepts::IsOutputCharRange<R>::_ &&
	Concepts::IsAsConsumableRange<VR>::_ &&
	!Meta::IsCharType<Concepts::ValueTypeOfAs<VR>>::_ &&
	Concepts::IsAsForwardCharRange<SR>::_ &&
	Concepts::IsAsCharRange<LR>::_ &&
	Concepts::IsAsCharRange<RR>::_
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
	Concepts::IsOutputCharRange<R>::_ &&
	Concepts::IsAsConsumableRange<VR>::_ &&
	!Meta::IsCharType<Concepts::ValueTypeOfAs<VR>>::_,
R&&> operator<<(R&& dst, VR&& r)
{
	ToString(dst, Range::Forward<VR>(r),
		StringView(", "), StringView("["), StringView("]"));
	return Cpp::Forward<R>(dst);
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
	Concepts::IsOutputCharRange<OR>::_ &&
	!Meta::IsArithmeticType<T>::_
> ToString(OR&& dst, T&& v) {dst << Cpp::Forward<T>(v);}

INTRA_WARNING_POP

}}
