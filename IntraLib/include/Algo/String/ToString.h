#pragma once

#include "Meta/Type.h"
#include "Meta/EachField.h"
#include "Range/Concepts.h"
#include "Range/Operations.h"
#include "Range/ForwardDecls.h"
#include "Range/Construction/Count.h"
#include "Range/Construction/Repeat.h"

namespace Intra { namespace Algo {

INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS

template<typename R, typename Char, size_t N> Meta::EnableIf<
	Range::IsOutputCharRange<R>::_ &&
	Meta::IsConvertible<Char, Range::ValueTypeOf<R>>::_
> ToString(R&& dst, const Char(&str)[N])
{for(Char c: str) dst.Put(Range::ValueTypeOf<R>(c));}

template<typename Char, size_t N> Meta::EnableIf<
	Meta::IsCharType<Char>::_,
size_t> MaxLengthOf(const Char(&str)[N]) {(void)str; return N;}

template<typename R> Meta::EnableIf<
	Range::IsOutputCharRange<R>::_
> ToStringCStr(R&& dst, const char* str)
{while(*str!='\0') dst.Put(Range::ValueTypeOf<R>(*str++));}

template<typename R, typename X, typename T=Range::ValueTypeOf<R>> Meta::EnableIf<
	Range::IsOutputCharRange<R>::_ &&
	Meta::IsUnsignedIntegralType<X>::_
> ToString(R&& dst, X number, int minWidth, T filler=' ', uint base=10, T minus='\0')
{
	INTRA_ASSERT(base>=2 && base<=36);
	T reversed[64];
	T* rev = reversed;
	do *rev++ = "0123456789abcdefghijklmnopqrstuvwxyz"[number%base], number = X(number/base);
	while(number!=0);
	if(minus) minWidth--;
	for(int i=0, s=int(minWidth-(rev-reversed)); i<s; i++)
		dst.Put(filler);
	if(minus) dst.Put(minus);
	while(rev!=reversed) dst.Put(*--rev);
}

template<typename T=char, typename X> static Meta::EnableIf<
	Meta::IsUnsignedIntegralType<X>::_,
size_t> MaxLengthOf(X number, int minWidth, T filler=' ', uint base=10, T minus='\0')
{
	(void)number; (void)filler;
	size_t maxLog;
	if(base<8) maxLog = sizeof(X)*8;
	else if(base<16) maxLog = (sizeof(X)*8+2)/3;
	else maxLog = sizeof(X)*2;
	return Math::Max(maxLog+(minus!='\0'), size_t(minWidth));
}

template<typename R, typename X> Meta::EnableIf<
	Range::IsOutputCharRange<R>::_ &&
	Meta::IsUnsignedIntegralType<X>::_ && sizeof(X)>=sizeof(size_t)
> ToString(R&& dst, X number)
{
	typedef Range::ValueTypeOf<R> T;
	T reversed[20];
	T* rev = reversed;
	do *rev++ = T(number%10+'0'), number/=10;
	while(number!=0);
	while(rev!=reversed) dst.Put(*--rev);
}

template<typename R, typename X> Meta::EnableIf<
	Range::IsOutputCharRange<R>::_ &&
	Meta::IsUnsignedIntegralType<X>::_ && sizeof(X)<sizeof(size_t)
> ToString(R&& dst, X number)
{
	ToString(dst, size_t(number));
}

template<typename X> static forceinline Meta::EnableIf<
	Meta::IsUnsignedIntegralType<X>::_,
size_t> MaxLengthOf(X number)
{
	(void)number;
	return sizeof(X)<2? 3: sizeof(X)==2? 5: sizeof(X)<=4? 10: sizeof(X)<=8? 18: 35;
}

template<typename R, typename X> forceinline Meta::EnableIf<
	Range::IsOutputCharRange<R>::_ &&
	Meta::IsSignedIntegralType<X>::_
> ToString(R&& dst, X number, int minWidth, Range::ValueTypeOf<R> filler=' ', uint base=10)
{
	ToString(dst, Meta::MakeUnsignedType<X>(number<0? -number: number),
		minWidth, filler, base, number<0? '-': '\0');
}


template<typename T=char, typename X> static forceinline Meta::EnableIf<
	Meta::IsSignedIntegralType<X>::_,
size_t> MaxLengthOf(X number, int minWidth, T filler=' ', uint base=10)
{
	return 1+MaxLengthOf(Meta::MakeUnsignedType<X>(number<0? -number: number),
		minWidth, filler, base, number<0? '-': '\0');
}

template<typename R, typename X> Meta::EnableIf<
	Range::IsOutputCharRange<R>::_&&
	Meta::IsSignedIntegralType<X>::_
> ToString(R&& dst, X number)
{
	if(number<0)
	{
		dst.Put('-');
		number = X(-number);
	}
	ToString(dst, Meta::MakeUnsignedType<X>(number));
}

template<typename X> static forceinline Meta::EnableIf<
	Meta::IsSignedIntegralType<X>::_,
size_t> MaxLengthOf(X number)
{
	(void)number;
	return sizeof(X)<2? 4: sizeof(X)==2? 6: sizeof(X)<=4? 11: sizeof(X)<=8? 18: 35;
}

template<typename R, typename X> Meta::EnableIf<
	Range::IsOutputCharRange<R>::_ &&
	Meta::IsUnsignedIntegralType<Meta::RemoveConstRef<X>>::_
> ToStringHexInt(R&& dst, X number)
{
	intptr digitPos = intptr(sizeof(X)*2);
	while(digitPos --> 0)
	{
		int value = int(number >> (digitPos*4)) & 15;
		if(value>9) value += 'A'-10;
		else value += '0';
		dst.Put(Range::ValueTypeOf<R>(value));
	}
}


template<typename R, typename X> forceinline Meta::EnableIf<
	Range::IsOutputCharRange<R>::_
> ToString(R&& dst, X* pointer)
{
	ToStringHexInt(dst, reinterpret_cast<size_t>(pointer));
}

template<typename X> static forceinline
size_t MaxLengthOf(X* pointer) {(void)pointer; return sizeof(X*)*2;}


template<typename R> Meta::EnableIf<
	Range::IsOutputCharRange<R>::_
> ToString(R&& dst, real number, int preciseness=15,
	Range::ValueTypeOf<R> dot='.', bool appendAllDigits=false)
{
	if(number==Math::NaN)
	{
		ToStringCStr(dst, "NaN");
		return;
	}
	if(number==Math::Infinity)
	{
		ToStringCStr(dst, "Infinity");
		return;
	}
	if(number==-Math::Infinity)
	{
		ToStringCStr(dst, "-Infinity");
		return;
	}

	if(number<0)
	{
		dst.Put('-');
		number = -number;
	}

	const ulong64 integralPart = ulong64(number);
	real fractional = number-real(integralPart);
	if(fractional>0.99)
	{
		ToString(dst, integralPart+1);
		fractional=0;
	}
	else ToString(dst, integralPart);

	if(preciseness==0) return;

	dst.Put(dot);
	do
	{
		fractional *= 10;
		int digit = int(fractional);
		fractional -= digit;
		if(fractional>0.99) fractional=0, digit++;
		dst.Put(Range::ValueTypeOf<R>('0'+digit));
	} while((fractional>=0.01 || appendAllDigits) && --preciseness>0);
}

template<typename R, typename X> Meta::EnableIf<
	Range::IsOutputCharRange<R>::_ &&
	Meta::IsFloatType<X>::_ && !Meta::TypeEquals<X, real>::_
> ToString(R&& dst, X number, int preciseness=sizeof(X)<=4? 7: 15,
	Range::ValueTypeOf<R> dot='.', bool appendAllDigits=false)
{
	ToString(dst, real(number), preciseness, dot, appendAllDigits);
}

template<typename T=char, typename X> static forceinline Meta::EnableIf<
	Meta::IsFloatType<X>::_,
size_t> MaxLengthOf(X number, int preciseness=2, T dot='.', bool appendAllDigits=false)
{
	(void)number; (void)dot; (void)appendAllDigits;
	return size_t(20+1+(preciseness+1));
}


template<typename R, typename Char2> forceinline Meta::EnableIf<
	Range::IsOutputCharRange<R>::_ &&
	Meta::IsCharType<Char2>::_
> ToString(R&& dst, Char2 character, size_t repeat=1)
{
	while(repeat --> 0) dst.Put(Range::ValueTypeOf<R>(character));
}

template<typename Char2> static forceinline Meta::EnableIf<
	Meta::IsCharType<Char2>::_,
size_t> MaxLengthOf(Char2 character, size_t repeat=1) {(void)character; return repeat;}


template<typename R, typename ForwardRange> forceinline Meta::EnableIf<
	Range::IsOutputCharRange<R>::_ &&
	Range::IsFiniteForwardRange<ForwardRange>::_ &&
	Range::ValueTypeEquals<ForwardRange, R>::_
> ToString(R&& dst, const ForwardRange& range)
{CopyAdvanceToAdvance(ForwardRange(range), dst);}

template<typename ForwardRange> static forceinline Meta::EnableIf<
	Range::IsFiniteForwardRange<ForwardRange>::_ &&
	Range::ValueTypeIsChar<ForwardRange>::_,
size_t> MaxLengthOf(const ForwardRange& range) {return Range::Count(range);}


template<typename R> Meta::EnableIf<
	Range::IsOutputCharRange<R>::_
> ToString(R&& dst, bool value)
{
	static const char* const boolStr[2] = {"false", "true"};
	const char* str = boolStr[+value];
	while(*str!='\0') dst.Put(*str++);
}

forceinline size_t MaxLengthOf(bool value) {(void)value; return 5;}


//Forward declarations

template<typename R, typename X> Meta::EnableIf<
	Range::IsOutputCharRange<R>::_ &&
	!Range::IsInputRange<X>::_ && Range::HasAsRange<X>::_
> ToString(R&& dst, const X& value);

template<typename X> static forceinline Meta::EnableIf<
	!Range::IsInputRange<X>::_ && Range::HasAsRange<X>::_,
size_t> MaxLengthOf(const X& value);

template<typename R, typename ForwardRange, typename OtherCharRange> Meta::EnableIf<
	Range::IsOutputRange<R>::_ &&
	Range::ValueTypeIsChar<R>::_ &&
	Range::IsFiniteForwardRange<ForwardRange>::_ &&
	!Range::ValueTypeIsChar<ForwardRange>::_ &&
	Range::IsForwardRange<OtherCharRange>::_ &&
	Range::ValueTypeIsChar<OtherCharRange>::_
> ToString(R&& dst, const ForwardRange& r,
	const OtherCharRange& separator,
	const OtherCharRange& lBracket,
	const OtherCharRange& rBracket);

template<typename ForwardRange, typename OtherCharRange> static forceinline Meta::EnableIf<
	Range::IsFiniteForwardRange<ForwardRange>::_ &&
	!Range::ValueTypeIsChar<ForwardRange>::_ &&
	!(Range::IsInputRange<Range::ValueTypeOf<ForwardRange>>::_ ||
		Range::IsInputRangeOfContainers<ForwardRange>::_ ||
		Range::ValueTypeIsTuple<ForwardRange>::_) &&
	Range::IsCharRange<OtherCharRange>::_,
size_t> MaxLengthOf(const ForwardRange& range,
	const OtherCharRange& separator,
	const OtherCharRange& lBracket,
	const OtherCharRange& rBracket);

template<typename ForwardRange, typename OtherCharRange> static forceinline Meta::EnableIf<
	Range::IsFiniteForwardRange<ForwardRange>::_ && !Range::ValueTypeIsChar<ForwardRange>::_ &&
	(Range::IsInputRange<Range::ValueTypeOf<ForwardRange>>::_ ||
		Range::IsInputRangeOfContainers<ForwardRange>::_  ||
		Range::ValueTypeIsTuple<ForwardRange>::_) &&
	Range::IsCharRange<OtherCharRange>::_,
size_t> MaxLengthOf(const ForwardRange& range,
	const OtherCharRange& separator,
	const OtherCharRange& lBracket,
	const OtherCharRange& rBracket);



namespace D {

template<typename Range, typename OtherCharRange> struct TupleAppender
{
	bool First;
	Range& DstRange;
	OtherCharRange Separator;

	TupleAppender(bool first, Range& dstRange, const OtherCharRange& elementSeparator):
		First(first), DstRange(dstRange), Separator(elementSeparator) {}

	TupleAppender& operator=(const TupleAppender&) = delete;

	template<typename V> void operator()(const V& value);
};

}

template<typename R, typename Tuple, typename OtherCharRange> Meta::EnableIf<
	Range::IsOutputCharRange<R>::_ &&
	Meta::IsTuple<Tuple>::_ &&
	Range::IsCharRange<OtherCharRange>::_
> ToString(R&& dst, const Tuple& tuple,
	const OtherCharRange& separator,
	const OtherCharRange& lBracket,
	const OtherCharRange& rBracket)
{
	ToString(dst, lBracket);
	D::TupleAppender<R, OtherCharRange> appender(true, dst, separator);
	Meta::ForEachField(tuple, appender);
	ToString(dst, rBracket);
}

template<typename Tuple, typename OtherCharRange> static Meta::EnableIf<
	Meta::HasForEachField<Tuple>::_ &&
	Range::IsCharRange<OtherCharRange>::_,
size_t> MaxLengthOf(const Tuple& tuple,
	const OtherCharRange& separator,
	const OtherCharRange& lBracket,
	const OtherCharRange& rBracket)
{
	Range::CountRange<char> counter;
	ToString(counter, lBracket);
	D::TupleAppender<Range::CountRange<char>, OtherCharRange> appender(false, counter, separator);
	Meta::ForEachField(tuple, appender);
	ToString(counter, rBracket);
	return counter.Counter;
}

template<typename R, typename Tuple> Meta::EnableIf<
	Range::IsOutputCharRange<R>::_ &&
	Meta::HasForEachField<Tuple>::_
> ToString(R&& dst, const Tuple& tuple)
{
	dst.Put('[');
	D::TupleAppender<R, StringView> appender(true, dst, ", ");
	Meta::ForEachField(tuple, appender);
	dst.Put(']');
}

template<typename Tuple> static Meta::EnableIf<
	Meta::HasForEachField<Tuple>::_,
size_t> MaxLengthOf(const Tuple& tuple)
{
	using namespace Range;
	CountRange<char> counter;
	D::TupleAppender<CountRange<char>, RTake<RRepeat<char>>> appender(true, counter, Repeat(' ', 2));
	Meta::ForEachField(tuple, appender);
	return 2+counter.Counter;
}

template<typename R, typename ForwardRange, typename OtherCharRange> Meta::EnableIf<
	Range::IsOutputRange<R>::_ && Range::ValueTypeIsChar<R>::_ &&
	Range::IsFiniteForwardRange<ForwardRange>::_ && !Range::ValueTypeIsChar<ForwardRange>::_ &&
	Range::IsForwardRange<OtherCharRange>::_ && Range::ValueTypeIsChar<OtherCharRange>::_
> ToString(R&& dst, const ForwardRange& r,
	const OtherCharRange& separator,
	const OtherCharRange& lBracket,
	const OtherCharRange& rBracket)
{
	ForwardRange range = r;
	Algo::ToString(dst, lBracket);
	if(!range.Empty())
	{
		auto v = range.First();
		Algo::ToString(dst, v);
		range.PopFirst();
	}
	while(!range.Empty())
	{
		Algo::ToString(dst, separator);
		Algo::ToString(dst, range.First());
		range.PopFirst();
	}
	Algo::ToString(dst, rBracket);
}

template<typename R, typename ForwardRange> Meta::EnableIf<
	Range::IsOutputRange<R>::_ && Range::ValueTypeIsChar<R>::_ &&
	Range::IsFiniteForwardRange<ForwardRange>::_ &&
	!Range::ValueTypeIsChar<ForwardRange>::_
> ToString(R&& dst, const ForwardRange& r)
{
	ToString(dst, r, AsRange({',', ' '}), AsRange({'['}), AsRange({']'}));
}

template<typename ForwardRange, typename OtherCharRange> static forceinline Meta::EnableIf<
	Range::IsFiniteForwardRange<ForwardRange>::_ && !Range::ValueTypeIsChar<ForwardRange>::_ &&
	!(Range::IsInputRange<Range::ValueTypeOf<ForwardRange>>::_ ||
		Range::IsInputRangeOfContainers<ForwardRange>::_ ||
		Range::ValueTypeIsTuple<ForwardRange>::_) &&
	Range::IsCharRange<OtherCharRange>::_,
size_t> MaxLengthOf(const ForwardRange& range,
	const OtherCharRange& separator,
	const OtherCharRange& lBracket,
	const OtherCharRange& rBracket)
{
	return (MaxLengthOf(Range::ValueTypeOf<ForwardRange>()) + Range::Count(separator))*Range::Count(range) +
		Range::Count(lBracket) + Range::Count(rBracket);
}

template<typename ForwardRange, typename OtherCharRange> static forceinline Meta::EnableIf<
	Range::IsFiniteForwardRange<ForwardRange>::_ && !Range::ValueTypeIsChar<ForwardRange>::_ &&
	(Range::IsInputRange<Range::ValueTypeOf<ForwardRange>>::_ ||
		Range::IsInputRangeOfContainers<ForwardRange>::_ ||
		Range::ValueTypeIsTuple<ForwardRange>::_) &&
	Range::IsCharRange<OtherCharRange>::_,
size_t> MaxLengthOf(const ForwardRange& range,
	const OtherCharRange& separator,
	const OtherCharRange& lBracket,
	const OtherCharRange& rBracket)
{
	size_t len=0;
	auto rangeCopy = range;
	while(!rangeCopy.Empty())
	{
		len += MaxLengthOf(rangeCopy.First());
		rangeCopy.PopFirst();
	}
	return len + Range::Count(separator)*Range::Count(range) +
		Range::Count(lBracket) + Range::Count(rBracket);
}

template<typename ForwardRange> static forceinline Meta::EnableIf<
	Range::IsFiniteForwardRange<ForwardRange>::_ &&
	!Range::ValueTypeIsChar<ForwardRange>::_,
size_t> MaxLengthOf(const ForwardRange& range)
{
	size_t len=0;
	auto rangeCopy = range;
	while(!rangeCopy.Empty())
	{
		len += MaxLengthOf(rangeCopy.First());
		rangeCopy.PopFirst();
	}
	return len + 2*Range::Count(range) + 2;
}

template<typename R, typename X> Meta::EnableIf<
	Range::IsOutputCharRange<R>::_ &&
	!Range::IsInputRange<X>::_ && Range::HasAsRange<X>::_
> ToString(R&& dst, const X& value)
{ToString(dst, value.AsRange());}

template<typename X> static forceinline Meta::EnableIf<
	!Range::IsInputRange<X>::_ && Range::HasAsRange<X>::_,
size_t> MaxLengthOf(const X& value) {return MaxLengthOf(value.AsRange());}

namespace D {

template<typename Range, typename OtherCharRange> template<typename V>
void TupleAppender<Range, OtherCharRange>::operator()(const V& value)
{
	if(!First) ToString(DstRange, Separator);
	ToString(DstRange, value);
	First = false;
}

}

INTRA_WARNING_POP

}}
