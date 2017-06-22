#pragma once

#include "Meta/Type.h"
#include "Concepts/Range.h"
#include "Concepts/RangeOf.h"

namespace Intra { namespace Range {

template<typename R, typename F,
	typename AsR = Concepts::RangeOfType<R>
> Meta::EnableIf<
	Concepts::IsConsumableRange<AsR>::_ &&
	Meta::IsCallable<F, Concepts::ValueTypeOf<AsR>>::_
> ForEach(R&& r, F&& f)
{
	auto range = Range::Forward<R>(r);
	while(!range.Empty())
	{
		f(range.First());
		range.PopFirst();
	}
}

template<typename R, typename FR,
	typename AsR = Concepts::RangeOfType<R>
> Meta::EnableIf<
	Concepts::IsConsumableRange<AsR>::_
> ForEach(R&& r, FR(Concepts::ValueTypeOf<AsR>::*f)())
{
	auto range = Range::Forward<R>(r);
	while(!range.Empty())
	{
		(range.First().*f)();
		range.PopFirst();
	}
}

template<typename R, typename FR,
	typename AsR = Concepts::RangeOfType<R>
> Meta::EnableIf<
	Concepts::IsConsumableRange<AsR>::_
> ForEach(R&& r, FR(Concepts::ValueTypeOf<AsR>::*f)() const)
{
	auto range = Range::Forward<R>(r);
	while(!range.Empty())
	{
		(range.First().*f)();
		range.PopFirst();
	}
}

template<typename R,
	typename AsR = Concepts::RangeOfType<R>, typename... Args
> Meta::EnableIf<
	Concepts::IsConsumableRange<AsR>::_ &&
	Meta::IsCallable<Concepts::ReturnValueTypeOf<AsR>, Args...>::_
> CallEach(R&& r, Args&&... args)
{
	auto range = Range::Forward<R>(r);
	while(!range.Empty())
	{
		range.First()(args...);
		range.PopFirst();
	}
}


}}
