#pragma once

#include "Intra/Concepts.h"
#include "Intra/Functional.h"
#include "Intra/Preprocessor.h"

INTRA_BEGIN
template<typename T> constexpr void Swap(T&& a, T&& b)
{
	if(&a == &b) return;
	auto temp = Move(a);
	a = Move(b);
	b = Move(temp);
}

template<typename T, typename U = T> constexpr T Exchange(T& dst, U&& newValue)
{
	T oldValue = Move(dst);
	dst = Forward<U>(newValue);
	return oldValue;
}

namespace z_D { namespace ADL {
void get(...) {}
}}


template<class Func> struct ForFieldAtRuntime: Func
{
	template<typename F> constexpr ForFieldAtRuntime(F&& f, Index index):
		Func(ForwardAsFunc<F>(f)), Index(index) {}

	template<class S, class = Requires<CStaticLengthContainer<S>>>
	constexpr auto operator()(S&& t)
	{
		return impl(Forward<S>(t), TMakeIndexSeq<StaticLength<S>>());
	}

private:
	INTRA_OPTIMIZE_FUNCTION(template<class S, size_t... Is>)
	constexpr auto impl(S&& t, TIndexSeq<Is...>)
	{
		INTRA_PRECONDITION(size_t(Index) < sizeof...(Is));
		using namespace z_D::ADL;
		using ReturnType = TMappedFieldCommon<S, Func>;
		if constexpr(CVoid<ReturnType>)
		{
			((size_t(Index) == Is &&
				(void(
					Func::operator()(get<Is>(Forward<S>(t)))
				), 1)) || ...);
		}
		else if constexpr(sizeof...(Is) <= 30)
		{
			//Efficient macro based solution which takes advantage of RVO and supports up to 30 fields
			#define INTRA_CASE_LINE(i) \
				if constexpr(i < sizeof...(Is)) if(size_t(Index) == i) return ReturnType(Func::operator()(get<i>(Forward<S>(t))));
			INTRA_MACRO_REPEAT(30, INTRA_CASE_LINE,);
			#undef INTRA_CASE_LINE
			return ReturnType();
		}
		else
		{
			//Less efficient solution (NRVO + move assignment instead of RVO), supports unlimited number of fields
			static_assert(CMoveConstructible<ReturnType> && (CConstructible<ReturnType> || CMoveAssignable<ReturnType>));
			TSelect<ReturnType, Optional<ReturnType>, CConstructible<ReturnType>> ret;
			((size_t(Index) == Is &&
				(void(
					ret = Func::operator()(get<Is>(Forward<S>(t)))
				), 1)) || ...);
			if constexpr(CSame<decltype(ret), ReturnType>) return ret;
			else return ret.Unwrap();
		}
	}
	INTRA_OPTIMIZE_FUNCTION_END

public:
	Index Index;
};
template<typename F> ForFieldAtRuntime(F, Index) -> ForFieldAtRuntime<TFunctorOf<F>>;

INTRA_END
