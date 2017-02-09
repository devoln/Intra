﻿#pragma once

#include "Platform/FundamentalTypes.h"
#include "Meta/Type.h"
#include "Range/Concepts.h"
#include "Range/Generators/Count.h"
#include "Range/AsRange.h"
#include "ToString.h"
#include "CStr.h"

namespace Intra { namespace Algo {

template<typename Char, size_t N> Meta::EnableIf<
	Meta::IsCharType<Char>::_,
size_t> MaxLengthOfToString(const Char(&str)[N]) {(void)str; return N;}


template<typename Char, size_t N> Meta::EnableIf<
	Meta::IsCharType<Char>::_,
size_t> MaxLengthOfToString(const Char* str) {return CStringLength(str);}

template<typename T=char, typename X> static Meta::EnableIf<
	Meta::IsUnsignedIntegralType<X>::_,
size_t> MaxLengthOfToString(X number, int minWidth, T filler=' ', uint base=10, T minus='\0')
{
	(void)number; (void)filler;
	size_t maxLog;
	if(base<8) maxLog = sizeof(X)*8;
	else if(base<16) maxLog = (sizeof(X)*8+2)/3;
	else maxLog = sizeof(X)*2;
	return Op::Max(maxLog+(minus!='\0'), size_t(minWidth));
}


template<typename X> static forceinline Meta::EnableIf<
	Meta::IsUnsignedIntegralType<X>::_,
size_t> MaxLengthOfToString(X number)
{
	(void)number;
	return sizeof(X)<2? 3: sizeof(X)==2? 5: sizeof(X)<=4? 10: sizeof(X)<=8? 18: 35;
}

template<typename T = char, typename X> static forceinline Meta::EnableIf<
	Meta::IsSignedIntegralType<X>::_,
size_t> MaxLengthOfToString(X number, int minWidth, T filler = ' ', uint base = 10)
{
	return 1+Algo::MaxLengthOfToString(Meta::MakeUnsignedType<X>(number<0? -number: number),
		minWidth, filler, base, number<0? '-': '\0');
}

template<typename X> static forceinline Meta::EnableIf<
	Meta::IsSignedIntegralType<X>::_,
size_t> MaxLengthOfToString(X number)
{
	(void)number;
	return sizeof(X)<2? 4: sizeof(X)==2? 6: sizeof(X)<=4? 11: sizeof(X)<=8? 18: 35;
}

template<typename X> static forceinline Meta::EnableIf<
	!Meta::IsCharType<X>::_,
size_t> MaxLengthOfToString(X* pointer) {(void)pointer; return sizeof(X*)*2;}


template<typename X, typename T=char> static forceinline Meta::EnableIf<
	Meta::IsFloatType<X>::_,
size_t> MaxLengthOfToString(X number, int preciseness=2, T dot='.', bool appendAllDigits=false)
{
	(void)number; (void)dot; (void)appendAllDigits;
	return size_t(20+1+(preciseness+1));
}

template<typename Char> static forceinline Meta::EnableIf<
	Meta::IsCharType<Char>::_,
size_t> MaxLengthOfToString(Char character, size_t repeat=1) {(void)character; return repeat;}


forceinline size_t MaxLengthOfToString(bool value) {return 4u+!value;}

template<typename SRC> static forceinline Meta::EnableIf<
	Range::IsAsNonInfiniteForwardRange<SRC>::_ &&
	Meta::IsCharType<Range::ValueTypeOfAs<SRC>>::_,
size_t> MaxLengthOfToString(SRC&& src) {return Range::Count(Range::Forward<SRC>(src));}


template<typename T> static forceinline Meta::EnableIf<
	!Meta::IsArithmeticType<T>::_ &&
	!Range::IsAsNonInfiniteForwardRange<T>::_,
size_t> MaxLengthOfToString(T&& v)
{
	Range::CountRange<char> counter;
	counter << Meta::Forward<T>(v);
	return counter.Counter;
}

namespace D {
template<typename VR> static forceinline
size_t MaxLengthOfImpl(VR&& r, size_t separatorLen);
}

template<typename VR> static forceinline Meta::EnableIf<
	Range::IsAsNonInfiniteForwardRange<VR>::_ &&
	!Meta::IsCharType<Range::ValueTypeOfAs<VR>>::_,
size_t> MaxLengthOfToString(VR&& r)
{return D::MaxLengthOfImpl(Range::Forward<VR>(r), 2)+2;}

namespace D {

template<typename VR> static forceinline
size_t MaxLengthOfImpl(VR&& r, size_t separatorLen)
{
	size_t result = 0;
	auto range = Range::Forward<VR>(r);
	if(!range.Empty()) result -= separatorLen;
	while(!range.Empty())
	{
		result += separatorLen;
		result += MaxLengthOfToString(range.First());
		range.PopFirst();
	}
	return result;
}

}

template<typename VR, typename SR, typename LR, typename RR> static forceinline Meta::EnableIf<
	Range::IsAsNonInfiniteForwardRange<VR>::_ &&
	Range::IsAsCharRange<SR>::_ && Range::IsAsCharRange<LR>::_ && Range::IsAsCharRange<SR>::_,
size_t> MaxLengthOfToString(VR&& r, SR&& separator, LR&& lBracket, RR&& rBracket)
{
	size_t result = Range::Count(Range::Forward<LR>(lBracket));
	result += Range::Count(Range::Forward<RR>(rBracket));
	result += D::MaxLengthOfImpl(Range::Forward<VR>(r), Range::Count(Range::Forward<SR>(separator)));
	return result;
}

template<typename VR, typename SR> static forceinline Meta::EnableIf<
	Range::IsAsNonInfiniteForwardRange<VR>::_ &&
	Meta::IsArithmeticType<Range::ValueTypeOfAs<VR>>::_ &&
	Range::IsAsCharRange<SR>::_,
size_t> MaxLengthOfToString(VR&& r, SR&& separator)
{return D::MaxLengthOfImpl(Meta::Forward<VR>(r), Range::Count(Range::Forward<SR>(separator)))+2;}

}}