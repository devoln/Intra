#pragma once

#include "Core/Range/Concepts.h"
#include "Core/Functional.h"

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

template<typename R> INTRA_NODISCARD constexpr forceinline Requires<
	CAsCharRange<R>,
size_t> CountLines(R&& range)
{
	auto rangeCopy = ForwardAsRange<R>(range);
	return CountLinesAdvance(rangeCopy);
}
INTRA_END
