#pragma once
#include "Intra/Type.h"
#include "Intra/Functional.h"

INTRA_BEGIN

//TODO

namespace ADL {
void Map();
void Reduce();
void Zip();
void StartsWith();
void EndsWith();
void Equals();
}

#define INTRA_FORWARD(rangeFunc) template<class U=T> auto rangeFunc() const -> decltype(rangeFunc(Val<const U&>())) \
	{using ADL::rangeFunc; return rangeFunc(*static_cast<const U*>(this));}
#define INTRA_FORWARD_FUNC(rangeFunc) template<typename F, class U=T> auto rangeFunc(F&& f) const \
	-> decltype(rangeFunc(Val<const U&>(), ForwardAsFunc<F>(f))) \
	{using ADL::rangeFunc; return rangeFunc(*static_cast<const U*>(this), ForwardAsFunc<F>(f));}
#define INTRA_FORWARD_VARIADIC(rangeFunc) template<typename... Ts, class U=T> auto rangeFunc(Ts&&... rs) const \
	-> decltype(rangeFunc(Val<const U&>(), INTRA_FWD(rs)...)) \
	{using ADL::rangeFunc; return rangeFunc(*static_cast<const U*>(this), INTRA_FWD(rs)...);}
#define INTRA_FORWARD1(rangeFunc) template<typename T0, class U=T> auto rangeFunc(T0&& x) const \
	-> decltype(rangeFunc(Val<const U&>(), INTRA_FWD(x))) \
	{using ADL::rangeFunc; return rangeFunc(*static_cast<const U*>(this), INTRA_FWD(x));}
#define INTRA_FORWARD_RANGE_FUNC(rangeFunc, defaultFuncType, defaultFuncValue) template<typename R, typename F defaultFuncType, class U=T> auto rangeFunc(R&& range, F&& func defaultFuncValue) const \
	-> decltype(rangeFunc(Val<const U&>(), INTRA_FWD(range), INTRA_FWD(func))) \
	{using ADL::rangeFunc; return rangeFunc(*static_cast<const U*>(this), INTRA_FWD(range), INTRA_FWD(func));}

template<typename T> struct RangeMethodsMixin
{
	INTRA_FORWARD(Count);
	INTRA_FORWARD_FUNC(Map);
	INTRA_FORWARD_FUNC(Reduce);
	INTRA_FORWARD_VARIADIC(Zip);
	INTRA_FORWARD_RANGE_FUNC(StartsWith, =decltype(Equal), =Equal);
	INTRA_FORWARD_RANGE_FUNC(EndsWith, =decltype(Equal), =Equal);
};

#undef INTRA_FORWARD_FUNC

INTRA_END
