#pragma once

#include "Range/Concepts.h"
#include "Algo/Op.h"

namespace Intra { namespace Algo {

template<typename R> Meta::EnableIf<
	Range::IsCharRange<R>::_ && !Meta::IsConst<R>::_,
size_t> SkipSpacesCountLinesAdvance(R&& src)
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
	Range::IsCharRange<R>::_,
size_t> CountLinesAdvance(R&& src)
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
	Range::IsCharRange<R>::_,
size_t> CountLines(const R& r)
{return CountLinesAdvance(R(r));}

}}

