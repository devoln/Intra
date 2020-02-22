#pragma once

#include "Core/Type.h"
#include "Core/Range/Concepts.h"
#include "Core/Range/Count.h"

#include "ToString.h"
#include "Core/Misc/RawMemory.h"

INTRA_BEGIN
template<typename Char, size_t N> Requires<
	CChar<Char>,
size_t> MaxLengthOfToString(const Char(&str)[N]) {(void)str; return N;}


template<typename Char, size_t N> Requires<
	CChar<Char>,
size_t> MaxLengthOfToString(const Char* str) {return Misc::CStringLength(str);}

template<typename T=char, typename X> Requires<
	CUnsignedIntegral<X>,
size_t> MaxLengthOfToString(X number, int minWidth, T filler=' ', uint base=10, T minus='\0')
{
	(void)number; (void)filler;
	size_t maxLog;
	if(base<8) maxLog = sizeof(X)*8;
	else if(base<16) maxLog = (sizeof(X)*8+2)/3;
	else maxLog = sizeof(X)*2;
	return FMax(maxLog + (minus != '\0'), size_t(minWidth));
}


template<typename X> forceinline Requires<
	CUnsignedIntegral<X>,
size_t> MaxLengthOfToString(X number)
{
	(void)number;
	return sizeof(X)<2? 3: sizeof(X)==2? 5: sizeof(X)<=4? 10: sizeof(X)<=8? 18: 35;
}

template<typename T = char, typename X> forceinline Requires<
	CSignedIntegral<X>,
size_t> MaxLengthOfToString(X number, int minWidth, T filler = ' ', uint base = 10)
{
	return 1+MaxLengthOfToString(TMakeUnsigned<X>(number<0? -number: number),
		minWidth, filler, base, number<0? '-': '\0');
}

template<typename X> forceinline Requires<
	CSignedIntegral<X> && !CChar<X>,
size_t> MaxLengthOfToString(X number)
{
	(void)number;
	return sizeof(X)<2? 4: sizeof(X)==2? 6: sizeof(X)<=4? 11: sizeof(X)<=8? 18: 35;
}

template<typename X> forceinline Requires<
	!CChar<X>,
size_t> MaxLengthOfToString(X* pointer) {(void)pointer; return sizeof(X*)*2;}

forceinline size_t MaxLengthOfToString(null_t) {return 4;}


template<typename X, typename T=char> forceinline Requires<
	CFloatingPoint<X>,
size_t> MaxLengthOfToString(X number, int preciseness=2, T dot='.', bool appendAllDigits=false)
{
	(void)number; (void)dot; (void)appendAllDigits;
	return size_t(20+1+(preciseness+1));
}

template<typename Char> forceinline Requires<
	CChar<Char>,
size_t> MaxLengthOfToString(Char character, size_t repeat=1) {(void)character; return repeat;}


forceinline size_t MaxLengthOfToString(bool value) {return 4u + !value;}

template<typename SRC,
	typename AsSRC = TRangeOfType<SRC>
> static forceinline Requires<
	CNonInfiniteForwardRange<AsSRC> &&
	CChar<TValueTypeOf<AsSRC>>,
size_t> MaxLengthOfToString(SRC&& src) {return Count(ForwardAsRange<SRC>(src));}

forceinline size_t MAxLengthOfToString(null_t) {return 4;}


template<typename T> static forceinline Requires<
	!CArithmetic<TRemoveReference<T>> &&
	!CSameIgnoreCVRef<T, null_t> &&
	!CAsNonInfiniteForwardRange<T>,
size_t> MaxLengthOfToString(T&& v)
{
	CountRange<char> counter;
	counter << Forward<T>(v);
	return counter.Counter;
}

namespace z__D {
template<typename VR> static forceinline
size_t MaxLengthOfImpl(VR&& r, size_t separatorLen);
}

template<typename VR,
	typename AsVR = TRangeOfType<VR>
> static forceinline Requires<
	CNonInfiniteForwardRange<AsVR> &&
	!CChar<TValueTypeOf<AsVR>>,
size_t> MaxLengthOfToString(VR&& r)
{return z__D::MaxLengthOfImpl(ForwardAsRange<VR>(r), 2)+2;}

namespace z__D {

template<typename VR> static forceinline
size_t MaxLengthOfImpl(VR&& r, size_t separatorLen)
{
	size_t result = 0;
	auto range = ForwardAsRange<VR>(r);
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

template<typename VR, typename SR, typename LR, typename RR> static forceinline Requires<
	CAsNonInfiniteForwardRange<VR> &&
	CAsCharRange<SR> &&
	CAsCharRange<LR> &&
	CAsCharRange<SR>,
size_t> MaxLengthOfToString(VR&& r, SR&& separator, LR&& lBracket, RR&& rBracket)
{
	size_t result = Count(ForwardAsRange<LR>(lBracket));
	result += Count(ForwardAsRange<RR>(rBracket));
	result += z__D::MaxLengthOfImpl(ForwardAsRange<VR>(r), Count(ForwardAsRange<SR>(separator)));
	return result;
}

template<typename VR, typename SR,
	typename AsVR = TRangeOfType<VR>
> static forceinline Requires<
	CNonInfiniteForwardRange<AsVR> &&
	CArithmetic<TValueTypeOf<AsVR>> &&
	CAsCharRange<SR>,
size_t> MaxLengthOfToString(VR&& r, SR&& separator)
{return z__D::MaxLengthOfImpl(Forward<VR>(r), Count(ForwardAsRange<SR>(separator)))+2;}
INTRA_END
