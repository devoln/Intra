#pragma once

#include "Concepts/Range.h"
#include "Concepts/RangeOf.h"
#include "Range/Decorators/Take.h"
#include "Range/Decorators/TakeUntil.h"
#include "Range/Search/Trim.h"
#include "Range/Search/Distance.h"
#include "Utils/Op.h"
#include "Cpp/Warnings.h"
#include "Range/Stream/Spaces.h"

namespace Intra { namespace Range {

INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS
INTRA_WARNING_DISABLE_SIGN_CONVERSION

template<typename R> Meta::EnableIf<
	Concepts::IsCharRange<R>::_ &&
	!Meta::IsConst<R>::_,
bool> ParseSignAdvance(R& src)
{
	bool minus = false;
	while(!src.Empty())
	{
		if(src.First()=='-') minus = !minus;
		else if(src.First()!='+' && !Op::IsSpace(src.First())) break;
		src.PopFirst();
	}
	return minus;
}

template<typename X, typename R> Meta::EnableIf<
	Concepts::IsCharRange<R>::_ &&
	!Meta::IsConst<R>::_ &&
	Meta::IsUnsignedIntegralType<X>::_,
X> ParseAdvance(R& src)
{
	TrimLeftAdvance(src, Op::IsSpace<Concepts::ValueTypeOf<R>>);
	X result = 0;
	while(!src.Empty())
	{
		uint digit = uint(src.First())-'0';
		if(digit>9) break;
		result = X(result*10+digit);
		src.PopFirst();
	}
	return result;
}

template<typename X, typename R> Meta::EnableIf<
	Concepts::IsCharRange<R>::_ &&
	!Meta::IsConst<R>::_ &&
	Meta::IsSignedIntegralType<X>::_,
X> ParseAdvance(R& src)
{
	const bool minus = ParseSignAdvance(src);
	X result = X(ParseAdvance<Meta::MakeUnsignedType<X>>(src));
	return minus? X(-result): result;
}

template<typename X, typename R> Meta::EnableIf<
	Concepts::IsCharRange<R>::_ &&
	!Meta::IsConst<R>::_ &&
	Meta::IsFloatType<X>::_,
X> ParseAdvance(R& src, Concepts::ValueTypeOf<R> decimalSeparator='.')
{
	X result = 0, pos = 1;
	bool waspoint = false;

	bool minus = ParseSignAdvance(src);

	for(; !src.Empty(); src.PopFirst())
	{
		Concepts::ValueTypeOf<R> c = src.First();
		if(c==decimalSeparator && !waspoint)
		{
			waspoint=true;
			continue;
		}
		uint digit = uint(c)-'0';
		if(digit>9) break;

		if(!waspoint) result = X(result*10+X(digit));
		else pos*=10, result += X(digit)/X(pos);
	}
	return minus? -result: result;
}

template<typename X, typename R> Meta::EnableIf<
	Concepts::IsCharRange<R>::_ &&
	!Meta::IsConst<R>::_ &&
	Meta::IsCharType<X>::_,
X> ParseAdvance(R& src)
{
	X result = src.First();
	src.PopFirst();
	return result;
}

template<typename R, typename CR> Meta::EnableIf<
	Concepts::IsCharRange<R>::_ &&
	!Meta::IsConst<R>::_ &&
	Concepts::IsCharRange<CR>::_ &&
	!Meta::IsConst<CR>::_,
bool> ExpectAdvance(R& src, CR& stringToExpect)
{
	TrimLeftAdvance(src, Op::IsSpace<Concepts::ValueTypeOf<R>>);
	TrimLeftAdvance(stringToExpect, Op::IsSpace<Concepts::ValueTypeOf<R>>);
	return StartsAdvanceWith(src, stringToExpect);
}

template<typename X, typename R,
	typename AsR = Concepts::RangeOfType<R>
> Meta::EnableIf<
	Concepts::IsConsumableRange<AsR>::_ &&
	Concepts::IsCharRange<AsR>::_ &&
	Meta::IsIntegralType<X>::_,
X> Parse(R&& src)
{
	auto range = Range::Forward<R>(src);
	return ParseAdvance<X>(range);
}

template<typename X, typename R,
	typename AsR = Concepts::RangeOfType<R>
> Meta::EnableIf<
	Concepts::IsConsumableRange<AsR>::_ &&
	Concepts::IsCharRange<AsR>::_ &&
	Meta::IsFloatType<X>::_,
X> Parse(R&& src, Concepts::ValueTypeOf<AsR> decimalSeparator = '.')
{
	auto range = Range::Forward<R>(src);
	return ParseAdvance<X>(range, decimalSeparator);
}

template<typename R, typename P1, typename P2> Meta::EnableIf<
	Concepts::IsCharRange<R>::_ &&
	Meta::IsCallable<P1, Concepts::ValueTypeOf<R>>::_ &&
	Meta::IsCallable<P2, Concepts::ValueTypeOf<R>>::_,
TakeResult<R>> ParseIdentifierAdvance(R& src, P1 isNotIdentifierFirstChar, P2 isNotIdentifierChar)
{
	TrimLeftAdvance(src, Op::IsHorSpace<char>);
	if(src.Empty() || isNotIdentifierFirstChar(src.First())) return null;
	auto result = src;
	src.PopFirst();
	while(!src.Empty() && !isNotIdentifierChar(src.First())) src.PopFirst();
	return Range::Take(result, DistanceTo(result, src));
}

template<typename R, typename X> Meta::EnableIf<
	Concepts::IsCharRange<R>::_ &&
	!Meta::IsConst<R>::_ &&
	Meta::IsArithmeticType<X>::_ &&
	!Meta::IsConst<X>::_,
R&&> operator>>(R&& stream, X&& dst)
{
	dst = ParseAdvance<Meta::RemoveReference<X>>(stream);
	return Cpp::Forward<R>(stream);
}

template<typename R, typename CR> Meta::EnableIf<
	Concepts::IsCharRange<R>::_ &&
	!Meta::IsConst<R>::_ &&
	Concepts::IsAsCharRange<CR>::_,
	// && Meta::IsConst<Concepts::ValueTypeOfAs<CR>>::_,
R&&> operator>>(R&& stream, CR&& stringToExpect)
{
	auto stringToExpectCopy = Range::Forward<CR>(stringToExpect);
	ExpectAdvance(stream, stringToExpectCopy);
	return Cpp::Forward<R>(stream);
}

INTRA_WARNING_POP

}}
