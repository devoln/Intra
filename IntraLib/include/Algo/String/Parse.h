#pragma once

#include "Range/Concepts.h"
#include "Range/Construction/Take.h"
#include "Algo/Search/Trim.h"
#include "Algo/Op.h"
#include "Algo/Search.h"

namespace Intra { namespace Algo {

template<typename R> Meta::EnableIf<
	Range::IsCharRange<R>::_,
bool> ParseSignAdvance(R&& src)
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
	Range::IsCharRange<R>::_ &&
	Meta::IsUnsignedIntegralType<X>::_,
X> ParseAdvance(R&& src)
{
	TrimLeftAdvance(src, Op::IsHorSpace<Range::ValueTypeOf<R>>);
	X result=0;
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
	Range::IsCharRange<R>::_ &&
	Meta::IsSignedIntegralType<X>::_,
X> ParseAdvance(R&& src)
{
	const bool minus = ParseSignAdvance(src);
	X result = X(ParseAdvance<Meta::MakeUnsignedType<X>>(src));
	return minus? X(-result): result;
}

template<typename X, typename R> Meta::EnableIf<
	Range::IsCharRange<R>::_ &&
	Meta::IsFloatType<X>::_,
X> ParseAdvance(R&& src, Range::ValueTypeOf<R> decimalSeparator='.')
{
	X result = 0, pos = 1;
	bool waspoint = false;

	bool minus = ParseSignAdvance(src);

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



template<typename R, typename P1, typename P2> Meta::EnableIf<
	Range::IsCharRange<R>::_ &&
	Meta::IsCallable<P1, Range::ValueTypeOf<R>>::_ &&
	Meta::IsCallable<P2, Range::ValueTypeOf<R>>::_,
Range::ResultOfTake<R>> ParseIdentifierAdvance(R&& src, P1 isNotIdentifierFirstChar, P2 isNotIdentifierChar)
{
	Algo::TrimLeftAdvance(src, Op::IsHorSpace<char>);
	if(src.Empty() || isNotIdentifierFirstChar(src.First())) return null;
	auto result = src;
	src.PopFirst();
	while(!src.Empty() && !isNotIdentifierChar(src.First())) src.PopFirst();
	return Range::Take(result, DistanceTo(result, src));
}


}}
