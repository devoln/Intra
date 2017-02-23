#pragma once

#include "Platform/CppWarnings.h"
#include "Platform/CppFeatures.h"
#include "Range/Concepts.h"
#include "Algo/Op.h"

namespace Intra { namespace Algo {

INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS

using namespace Range::Concepts;

template<typename R> Meta::EnableIf<
	IsCharRange<R>::_ && !Meta::IsConst<R>::_,
size_t> SkipSpacesCountLinesAdvance(R& src)
{
	size_t lines = 0;
	bool wasCR = false;
	while(!src.Empty() && Op::IsSpace(src.First()))
	{
		char c = src.First();
		if(c=='\r' || (c=='\n' && !wasCR))
			lines++;
		wasCR = (c=='\r');
		src.PopFirst();
	}
	return lines;
}

template<typename R> Meta::EnableIf<
	IsCharRange<R>::_ && !Meta::IsConst<R>::_,
size_t> CountLinesAdvance(R& src)
{
	size_t lines = 0;
	bool wasCR = false;
	while(!src.Empty())
	{
		char c = src.First();
		if(c=='\r' || (c=='\n' && !wasCR))
			lines++;
		wasCR = c=='\r';
		src.PopFirst();
	}
	return lines;
}

template<typename R> forceinline Meta::EnableIf<
	IsAsCharRange<R>::_,
size_t> CountLines(R&& range)
{
	auto rangeCopy = Range::Forward<R>(range);
	return CountLinesAdvance(rangeCopy);
}

INTRA_WARNING_POP

}}

