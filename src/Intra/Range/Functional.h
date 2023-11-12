#pragma once

#include <Intra/Functional.h>
#include <Intra/Concepts.h>

namespace Intra { INTRA_BEGIN

template<class R, typename P = decltype(Equal)> class IsOneOf: P
{
	static_assert(CNonInfiniteForwardRange<R>);
	R mSet;
	R mFound;
public:
	constexpr IsOneOf(R set): mSet(Move(set)) {}
	constexpr IsOneOf(R set, P pred): P(Move(pred)), mSet(Move(set)) {}

	template<typename T> requires CCallable<P, T&&>
	[[nodiscard]] constexpr bool operator()(T&& arg)
	{
		mFound = mSet;
		while(!mFound.Empty() && !P::operator()(mFound.First())) mFound.PopFirst();
		return !mFound.Empty();
	}

	auto Which() const {return mFound.First();}
};
template<class R> IsOneOf(R) -> IsOneOf<TRangeOfRef<R>>;
template<class R, class P> IsOneOf(R, P) -> IsOneOf<TRangeOfRef<R>, TFunctorOf<P>>;

} INTRA_END
