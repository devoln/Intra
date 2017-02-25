#pragma once

#include "Platform/FundamentalTypes.h"
#include "Platform/CppFeatures.h"
#include "Platform/CppWarnings.h"
#include "Pair.h"
#include "Tuple.h"

INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS

namespace Intra { namespace Meta {

namespace D {

template<size_t I> struct Tuple_GetN_T
{
	template<typename T0, typename ...Args> static forceinline
	auto cfunc(const Tuple<T0, Args...>& t) -> decltype(Tuple_GetN_T<I-1>::cfunc(t.next))
	{return Tuple_GetN_T<I-1>::cfunc(t.next);}

	template<typename T0, typename ...Args> static forceinline
	auto func(Tuple<T0, Args...>& t) -> decltype(Tuple_GetN_T<I-1>::func(t.next))
	{return Tuple_GetN_T<I-1>::func(t.next);}
};

template<> struct Tuple_GetN_T<0>
{
	template<typename T0, typename ...Args> static forceinline
	const T0& cfunc(const Tuple<T0, Args...>& t) {return t.first;}

	template<typename T0, typename ...Args> static forceinline
	T0& func(Tuple<T0, Args...>& t) {return t.first;}
};

template<size_t I> struct Pair_Get_T;
template<> struct Pair_Get_T<0>
{
	template<typename P> static forceinline auto get(P&& p) -> EnableIf<
		Has_first_second<P>::_,
	decltype(*&p.first)>
	{return p.first;}

	template<typename P> static forceinline auto get(P&& p) -> EnableIf<
		HasKeyValue<P>::_,
	decltype(*&p.Key)>
	{return p.Key;}
};

template<> struct Pair_Get_T<1>
{
	template<typename P> static forceinline auto get(P&& p) -> EnableIf<
		Has_first_second<P>::_,
	decltype(*&p.second)>
	{return p.second;}

	template<typename P> static forceinline auto get(P&& p) -> EnableIf<
		HasKeyValue<P>::_,
	decltype(*&p.Value)>
	{return p.Value;}
};

}


template<size_t N, typename... Types> forceinline
TypeListAt<N, TypeList<Types...>>& Get(Tuple<Types...>& tuple)
{return D::Tuple_GetN_T<N>::func(tuple);}

template<size_t N, typename... Types> forceinline
const TypeListAt<N, TypeList<Types...>>& Get(const Tuple<Types...>& tuple)
{return D::Tuple_GetN_T<N>::cfunc(tuple);}


template<size_t N, typename P> forceinline auto Get(P&& p) -> EnableIf<
	Has_first_second<P>::_ || HasKeyValue<P>::_,
decltype(D::Pair_Get_T<N>::get(Meta::Forward<P>(p)))>
{return D::Pair_Get_T<N>::get(Meta::Forward<P>(p));}


template<size_t N, typename T> struct TupleElementEqualsFunctor
{
	const T& Value;
	forceinline TupleElementEqualsFunctor(const T& value): Value(value) {}
	template<typename TUPLE> forceinline bool operator()(const TUPLE& rhs) const {return Value==Get<N>(rhs);}
};

template<size_t N, typename T> forceinline TupleElementEqualsFunctor<N, T> TupleElementEquals(const T& value)
{return TupleElementEqualsFunctor<N, T>(value);}

}}

INTRA_WARNING_POP
