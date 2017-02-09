#pragma once

#include "Range/Concepts.h"
#include "Range/AsRange.h"
#include "Range/Decorators/Take.h"
#include "Range/Decorators/TakeUntil.h"
#include "Algo/Search/Trim.h"
#include "Algo/Search/Distance.h"
#include "Algo/Op.h"
#include "Platform/CppWarnings.h"
#include "Algo/String/Spaces.h"

namespace Intra { namespace Algo {

INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS
INTRA_WARNING_DISABLE_SIGN_CONVERSION

template<typename R> Meta::EnableIf<
	Range::IsCharRange<R>::_ && !Meta::IsConst<R>::_,
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
	Range::IsCharRange<R>::_ && !Meta::IsConst<R>::_ &&
	Meta::IsUnsignedIntegralType<X>::_,
X> ParseAdvance(R& src)
{
	Algo::TrimLeftAdvance(src, Op::IsSpace<Range::ValueTypeOf<R>>);
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
	Range::IsCharRange<R>::_ && !Meta::IsConst<R>::_ &&
	Meta::IsSignedIntegralType<X>::_,
X> ParseAdvance(R& src)
{
	const bool minus = ParseSignAdvance(src);
	X result = X(Algo::ParseAdvance<Meta::MakeUnsignedType<X>>(src));
	return minus? X(-result): result;
}

template<typename X, typename R> Meta::EnableIf<
	Range::IsCharRange<R>::_ && !Meta::IsConst<R>::_ &&
	Meta::IsFloatType<X>::_,
X> ParseAdvance(R& src, Range::ValueTypeOf<R> decimalSeparator='.')
{
	X result = 0, pos = 1;
	bool waspoint = false;

	bool minus = Algo::ParseSignAdvance(src);

	for(; !src.Empty(); src.PopFirst())
	{
		Range::ValueTypeOf<R> c = src.First();
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
	Range::IsCharRange<R>::_ && !Meta::IsConst<R>::_ &&
	Meta::IsCharType<X>::_,
X> ParseAdvance(R& src)
{
	X result = src.First();
	src.PopFirst();
	return result;
}

template<typename R, typename CR> Meta::EnableIf<
	Range::IsCharRange<R>::_ && !Meta::IsConst<R>::_ &&
	Range::IsCharRange<CR>::_ && !Meta::IsConst<CR>::_,
bool> ExpectAdvance(R& src, CR& stringToExpect)
{
	Algo::TrimLeftAdvance(src, Op::IsSpace<Range::ValueTypeOf<R>>);
	Algo::TrimLeftAdvance(stringToExpect, Op::IsSpace<Range::ValueTypeOf<R>>);
	return Algo::StartsAdvanceWith(src, stringToExpect);
}

template<typename X, typename R> Meta::EnableIf<
	Range::IsAsConsumableRange<R>::_ && Range::IsAsCharRange<R>::_ &&
	Meta::IsIntegralType<X>::_,
X> Parse(R&& src)
{
	auto range = Range::Forward<R>(src);
	return Algo::ParseAdvance<X>(range);
}

template<typename X, typename R> Meta::EnableIf<
	Range::IsAsConsumableRange<R>::_ && Range::IsAsCharRange<R>::_ &&
	Meta::IsFloatType<X>::_,
X> Parse(R&& src, Range::ValueTypeOfAs<R> decimalSeparator = '.')
{
	auto range = Range::Forward<R>(src);
	return Algo::ParseAdvance<X>(range, decimalSeparator);
}

template<typename R, typename P1, typename P2> Meta::EnableIf<
	Range::IsCharRange<R>::_ &&
	Meta::IsCallable<P1, Range::ValueTypeOf<R>>::_ &&
	Meta::IsCallable<P2, Range::ValueTypeOf<R>>::_,
Range::ResultOfTake<R>> ParseIdentifierAdvance(R& src, P1 isNotIdentifierFirstChar, P2 isNotIdentifierChar)
{
	Algo::TrimLeftAdvance(src, Op::IsHorSpace<char>);
	if(src.Empty() || isNotIdentifierFirstChar(src.First())) return null;
	auto result = src;
	src.PopFirst();
	while(!src.Empty() && !isNotIdentifierChar(src.First())) src.PopFirst();
	return Range::Take(result, Algo::DistanceTo(result, src));
}

template<typename R, typename X> Meta::EnableIf<
	Range::IsCharRange<R>::_ && !Meta::IsConst<R>::_ &&
	Meta::IsArithmeticType<X>::_ && !Meta::IsConst<X>::_,
R&&> operator>>(R&& stream, X&& dst)
{
	dst = Algo::ParseAdvance<Meta::RemoveReference<X>>(stream);
	return Meta::Forward<R>(stream);
}

template<typename R, typename CR> Meta::EnableIf<
	Range::IsCharRange<R>::_ && !Meta::IsConst<R>::_ &&
	Range::IsAsCharRange<CR>::_,// && Meta::IsConst<Range::ValueTypeOfAs<CR>>::_,
R&&> operator>>(R&& stream, CR&& stringToExpect)
{
	auto stringToExpectCopy = Range::Forward<CR>(stringToExpect);
	Algo::ExpectAdvance(stream, stringToExpectCopy);
	return Meta::Forward<R>(stream);
}

INTRA_WARNING_POP

}}
