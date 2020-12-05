#pragma once

#include "Intra/Range/Concepts.h"
#include "Intra/Functional.h"

INTRA_BEGIN
template<typename R> constexpr Requires<
	CCharRange<R> &&
	!CConst<R>,
size_t> SkipSpacesCountLinesAdvance(R& src)
{
	size_t lines = 0;
	bool wasCR = false;
	while(!src.Empty() && IsSpace(src.First()))
	{
		char c = src.First();
		if(c=='\r' || (c=='\n' && !wasCR))
			lines++;
		wasCR = (c=='\r');
		src.PopFirst();
	}
	return lines;
}

template<typename R> constexpr Requires<
	CCharRange<R> &&
	!CConst<R>,
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

template<typename R> [[nodiscard]] constexpr Requires<
	CCharList<R>,
size_t> CountLines(R&& range)
{
	auto rangeCopy = ForwardAsRange<R>(range);
	return CountLinesAdvance(rangeCopy);
}
INTRA_END
