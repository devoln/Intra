#pragma once

#include "Platform/CppWarnings.h"
#include "Meta/Type.h"
#include "Meta/Pair.h"
#include "Meta/Tuple.h"

namespace Intra { namespace Meta {

INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS

template<typename Func, typename Arg0, typename Arg1, typename... Args> forceinline
void ForEachField(Tuple<Arg0, Arg1, Args...>& tuple, Func&& f)
{
	f(tuple.first);
	ForEachField(tuple.next, Meta::Forward<Func>(f));
}

template<typename Func, typename Arg0> forceinline
void ForEachField(Tuple<Arg0>& tuple, Func&& f)
{f(tuple.first);}

template<typename Func, typename Arg0, typename Arg1, typename... Args> forceinline
void ForEachField(const Tuple<Arg0, Arg1, Args...>& tuple, Func&& f)
{
	f(tuple.first);
	ForEachField(tuple.next, Meta::Forward<Func>(f));
}

template<typename Func, typename Arg0> forceinline
void ForEachField(const Tuple<Arg0>& tuple, Func&& f)
{f(tuple.first);}

template<typename Func, typename Arg0> forceinline
Tuple<ResultOf<Func, Arg0>>
TransformEachField(const Tuple<Arg0>& args, Func&& f)
{return TupleL(f(args.first));}

template<typename Func, typename Arg0, typename Arg1, typename... Args> forceinline
Tuple<ResultOf<Func, Arg0>, ResultOf<Func, Arg1>, ResultOf<Func, Args>...>
TransformEachField(const Tuple<Arg0, Arg1, Args...>& args, Func&& f)
{
	typedef Tuple<ResultOf<Func, Arg0>, ResultOf<Func, Arg1>, ResultOf<Func, Args>...> ResultType;
	return ResultType(f(args.first), TransformEachField(args.next, f));
}


template<typename F, typename K, typename V> forceinline
void ForEachField(KeyValuePair<K,V>& pair, F&& f)
{f(pair.Key); f(pair.Value);}

template<typename F, typename K, typename V> forceinline
void ForEachField(const KeyValuePair<K,V>& pair, F&& f)
{f(pair.Key); f(pair.Value);}

template<typename F, typename K, typename V> forceinline
KeyValuePair<ResultOf<F, K>, ResultOf<F, V>> TransformEachField(const KeyValuePair<K, V>& pair, F&& f)
{return {f(pair.Key), f(pair.Value)};}


template<typename F, typename P> forceinline Meta::EnableIf<
	Has_first_second<P>::_
> ForEachField(P&& pair, F&& f)
{f(pair.first); f(pair.second);}


namespace D {
INTRA_DEFINE_EXPRESSION_CHECKER2(HasForEachFieldMethod, Val<T1>().ForEachField(Val<T2>()), , = UniFunctor);
}
template<typename T, typename F=UniFunctor> struct HasForEachFieldMethod:
	D::HasForEachFieldMethod<RemoveReference<T>, RemoveReference<F>> {};

template<typename Func, typename T> forceinline EnableIf<
	HasForEachFieldMethod<T, Func>::_
> ForEachField(T&& value, Func&& f)
{value.ForEachField(Forward<Func>(f));}

namespace D {
INTRA_DEFINE_EXPRESSION_CHECKER2(HasForEachField, ForEachField(Val<T1>(), Val<T2>()), , = UniFunctor);
}
template<typename T, typename F=UniFunctor> struct HasForEachField:
	D::HasForEachField<RemoveReference<T>, RemoveReference<F>> {};

INTRA_WARNING_POP

}}
