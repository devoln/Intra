#pragma once

#include "Intra/Functional.h"
#include "Intra/Range/Concepts.h"
#include "Intra/Range/Take.h"
#include "Intra/Range/TakeUntil.h"
#include "Intra/Range/Search/Trim.h"
#include "Intra/Range/Search/Distance.h"
#include "Intra/Range/Stream/Spaces.h"

INTRA_BEGIN
INTRA_IGNORE_WARN_SIGN_CONVERSION
template<typename R> constexpr Requires<
	CCharRange<R> &&
	!CConst<R>,
bool> ParseSignAdvance(R& src)
{
	bool minus = false;
	while(!src.Empty())
	{
		if(src.First()=='-') minus = !minus;
		else if(src.First() != '+' && !IsSpace(src.First())) break;
		src.PopFirst();
	}
	return minus;
}

template<typename X, typename R> constexpr Requires<
	CCharRange<R> &&
	!CConst<R> &&
	CUnsignedIntegral<X>,
X> ParseAdvance(R& src)
{
	TrimLeftAdvance(src, IsSpace);
	X result = 0;
	while(!src.Empty())
	{
		unsigned digit = unsigned(src.First())-'0';
		if(digit>9) break;
		result = X(result*10+digit);
		src.PopFirst();
	}
	return result;
}

template<typename X, typename R> constexpr Requires<
	CCharRange<R> &&
	!CConst<R> &&
	CSignedIntegral<X>,
X> ParseAdvance(R& src)
{
	const bool minus = ParseSignAdvance(src);
	X result = X(ParseAdvance<TToUnsigned<X>>(src));
	return minus? X(-result): result;
}

template<typename X, typename R> constexpr Requires<
	CCharRange<R> &&
	!CConst<R> &&
	CFloatingPoint<X>,
X> ParseAdvance(R& src, TRangeValue<R> decimalSeparator='.')
{
	X result = 0, pos = 1;
	bool waspoint = false;

	bool minus = ParseSignAdvance(src);

	for(; !src.Empty(); src.PopFirst())
	{
		TRangeValue<R> c = src.First();
		if(c==decimalSeparator && !waspoint)
		{
			waspoint=true;
			continue;
		}
		unsigned digit = unsigned(c)-'0';
		if(digit>9) break;

		if(!waspoint) result = X(result*10+X(digit));
		else pos*=10, result += X(digit)/X(pos);
	}
	return minus? -result: result;
}

template<typename X, typename R> constexpr Requires<
	CCharRange<R> &&
	!CConst<R> &&
	CChar<X>,
X> ParseAdvance(R& src)
{
	X result = src.First();
	src.PopFirst();
	return result;
}

/// Сопоставляет начало исходного потока src со stringToExpect, игнорируя пробелы в начале src и stringToExpect.
/// В случае совпадения сдвигает начало src в конец вхождения stringToExpect, а stringToExpect сдвигает в конец, делая его пустым.
/// В случае несовпадения удаляет только пробелы из начала обоих потоков.
/// @return Возвращает true в случае совпадения src и stringToExpect.
template<typename R, typename CR> constexpr Requires<
	CCharRange<R> &&
	!CConst<R> &&
	CCharRange<CR> &&
	!CConst<CR>,
bool> ExpectAdvance(R& src, CR& stringToExpect)
{
	TrimLeftAdvance(src, IsSpace);
	TrimLeftAdvance(stringToExpect, IsSpace);
	auto srcCopy = src;
	if(StartsAdvanceWith(srcCopy, stringToExpect))
	{
		src = srcCopy;
		return true;
	}
	return false;
}

template<typename X, typename R,
	typename AsR = TRangeOfRef<R>
> [[nodiscard]] constexpr Requires<
	CConsumableRange<AsR> &&
	CCharRange<AsR> &&
	CIntegral<X>,
X> Parse(R&& src)
{
	auto range = ForwardAsRange<R>(src);
	return ParseAdvance<X>(range);
}

template<typename X, typename R,
	typename AsR = TRangeOfRef<R>
> [[nodiscard]] constexpr Requires<
	CConsumableRange<AsR> &&
	CCharRange<AsR> &&
	CFloatingPoint<X>,
X> Parse(R&& src, TRangeValue<AsR> decimalSeparator = '.')
{
	auto range = ForwardAsRange<R>(src);
	return ParseAdvance<X>(range, decimalSeparator);
}

template<typename R, typename P1, typename P2> constexpr Requires<
	CCharRange<R> &&
	CCallable<P1, TRangeValue<R>> &&
	CCallable<P2, TRangeValue<R>>,
TTakeResult<R>> ParseIdentifierAdvance(R& src, P1 isNotIdentifierFirstChar, P2 isNotIdentifierChar)
{
	TrimLeftAdvance(src, IsHorSpace);
	if(src.Empty() || isNotIdentifierFirstChar(src.First())) return null;
	auto result = src;
	src.PopFirst();
	while(!src.Empty() && !isNotIdentifierChar(src.First())) src.PopFirst();
	return Take(result, DistanceTo(result, src));
}

template<typename R, typename X> constexpr Requires<
	CCharRange<R> &&
	!CConst<TRemoveReference<X>> &&
	CArithmetic<TRemoveReference<X>> &&
	!CConst<TRemoveReference<X>>,
R&&> operator>>(R&& stream, X&& dst)
{
	dst = ParseAdvance<TRemoveReference<X>>(stream);
	return Forward<R>(stream);
}

template<typename R, typename CR> constexpr Requires<
	CCharRange<R> &&
	!CConst<R> &&
	CCharList<CR>,
	// && CConst<TListValue<CR>>,
R&&> operator>>(R&& stream, CR&& stringToExpect)
{
	auto stringToExpectCopy = ForwardAsRange<CR>(stringToExpect);
	ExpectAdvance(stream, stringToExpectCopy);
	return Forward<R>(stream);
}
INTRA_END
