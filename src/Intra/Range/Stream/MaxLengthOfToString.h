#pragma once

#include "Intra/Type.h"
#include "Intra/Range/Concepts.h"
#include "Intra/Range/Count.h"

#include "ToString.h"
#include "Intra/Misc/RawMemory.h"

INTRA_BEGIN
template<typename Char, index_t N> [[nodiscard]] constexpr Requires<
	CChar<Char>,
index_t> MaxLengthOfToString(const Char(&str)[N]) {(void)str; return N;}


template<typename Char> [[nodiscard]] constexpr Requires<
	CChar<Char>,
index_t> MaxLengthOfToString(const Char* str) {return Misc::CStringLength(str);}

template<typename T=char, typename X> [[nodiscard]] constexpr Requires<
	CUnsignedIntegral<X>,
index_t> MaxLengthOfToString(X number, int minWidth, T filler=' ', unsigned base=10, T minus='\0')
{
	(void)number; (void)filler;
	index_t maxLog = sizeof(X)*2;
	if(base < 8) maxLog = sizeof(X)*8;
	else if(base < 16) maxLog = (sizeof(X)*8+2)/3;
	return FMax(maxLog + (minus != '\0'), index_t(minWidth));
}


template<typename X> [[nodiscard]] constexpr Requires<
	CUnsignedIntegral<X>,
index_t> MaxLengthOfToString(X number)
{
	(void)number;
	return sizeof(X) < 2? 3: sizeof(X) == 2? 5: sizeof(X) <= 4? 10: sizeof(X) <= 8? 18: 35;
}

template<typename T = char, typename X> [[nodiscard]] constexpr Requires<
	CSignedIntegral<X>,
index_t> MaxLengthOfToString(X number, int minWidth, T filler = ' ', unsigned base = 10)
{
	return 1 + MaxLengthOfToString(TToUnsigned<X>(number<0? -number: number),
		minWidth, filler, base, number<0? '-': '\0');
}

template<typename X> [[nodiscard]] constexpr Requires<
	CSignedIntegral<X> && !CChar<X>,
index_t> MaxLengthOfToString(X number)
{
	(void)number;
	return sizeof(X) < 2? 4: sizeof(X) == 2? 6: sizeof(X) <= 4? 11: sizeof(X) <= 8? 18: 35;
}

template<typename X> [[nodiscard]] constexpr Requires<
	!CChar<X>,
index_t> MaxLengthOfToString(X* pointer) {(void)pointer; return sizeof(X*)*2;}

[[nodiscard]] constexpr index_t MaxLengthOfToString(decltype(null)) {return 4;}


template<typename X, typename T=char> [[nodiscard]] constexpr Requires<
	CFloatingPoint<X>,
index_t> MaxLengthOfToString(X number, int preciseness=2, T dot='.', bool appendAllDigits=false)
{
	(void)number; (void)dot; (void)appendAllDigits;
	return 20 + 1 + (preciseness + 1);
}

template<typename Char> [[nodiscard]] constexpr Requires<
	CChar<Char>,
index_t> MaxLengthOfToString(Char character, index_t repeat = 1) {(void)character; return repeat;}


[[nodiscard]] constexpr index_t MaxLengthOfToString(bool value) {return 4u + !value;}

template<typename SRC,
	typename AsSRC = TRangeOfRef<SRC>
> [[nodiscard]] constexpr Requires<
	CNonInfiniteForwardRange<AsSRC> &&
	CChar<TValueTypeOf<AsSRC>>,
index_t> MaxLengthOfToString(SRC&& src) {return Count(ForwardAsRange<SRC>(src));}


template<typename T> static constexpr Requires<
	!CArithmetic<TRemoveReference<T>> &&
	!CSameIgnoreCVRef<T, decltype(null)> &&
	!CAsNonInfiniteForwardRange<T>,
index_t> MaxLengthOfToString(T&& v)
{
	CountRange<char> counter;
	counter << Forward<T>(v);
	return counter.Counter;
}

namespace z_D {
template<typename VR> static constexpr
index_t MaxLengthOfImpl(VR&& r, index_t separatorLen);
}

template<typename VR,
	typename AsVR = TRangeOfRef<VR>
> static constexpr Requires<
	CNonInfiniteForwardRange<AsVR> &&
	!CChar<TValueTypeOf<AsVR>>,
index_t> MaxLengthOfToString(VR&& r)
{return z_D::MaxLengthOfImpl(ForwardAsRange<VR>(r), 2)+2;}

namespace z_D {

template<typename VR> static constexpr
index_t MaxLengthOfImpl(VR&& r, index_t separatorLen)
{
	index_t result = 0;
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

template<typename VR, typename SR, typename LR, typename RR> static constexpr Requires<
	CAsNonInfiniteForwardRange<VR> &&
	CAsCharRange<SR> &&
	CAsCharRange<LR> &&
	CAsCharRange<SR>,
index_t> MaxLengthOfToString(VR&& r, SR&& separator, LR&& lBracket, RR&& rBracket)
{
	auto result = Count(ForwardAsRange<LR>(lBracket));
	result += Count(ForwardAsRange<RR>(rBracket));
	result += z_D::MaxLengthOfImpl(ForwardAsRange<VR>(r), Count(ForwardAsRange<SR>(separator)));
	return result;
}

template<typename VR, typename SR,
	typename AsVR = TRangeOfRef<VR>
> static constexpr Requires<
	CNonInfiniteForwardRange<AsVR> &&
	CArithmetic<TValueTypeOf<AsVR>> &&
	CAsCharRange<SR>,
index_t> MaxLengthOfToString(VR&& r, SR&& separator)
{return z_D::MaxLengthOfImpl(Forward<VR>(r), Count(ForwardAsRange<SR>(separator)))+2;}
INTRA_END
