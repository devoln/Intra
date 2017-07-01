#pragma once

#include "Cpp/Warnings.h"
#include "Cpp/Features.h"

#include "Concepts/Range.h"

#include "Funal/Op.h"

namespace Intra { namespace Range {

INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS

template<typename R> Meta::EnableIf<
	Concepts::IsCharRange<R>::_ &&
	!Meta::IsConst<R>::_,
size_t> SkipSpacesCountLinesAdvance(R& src)
{
	size_t lines = 0;
	bool wasCR = false;
	while(!src.Empty() && Funal::IsSpace(src.First()))
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
	Concepts::IsCharRange<R>::_ &&
	!Meta::IsConst<R>::_,
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
	Concepts::IsAsCharRange<R>::_,
size_t> CountLines(R&& range)
{
	auto rangeCopy = Range::Forward<R>(range);
	return CountLinesAdvance(rangeCopy);
}

INTRA_WARNING_POP

}}

