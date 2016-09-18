#pragma once

#include "Core/Core.h"
#include "Meta/TypeList.h"

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable: 4512 4626 5026 5027)
#endif

namespace Intra { namespace Meta {

template<typename ...Args> struct Tuple;


namespace detail {
template<int...> struct index_tuple {};
template<int I, typename IndexTuple, typename... Types> struct make_indexes_impl;

template<int I, int... Indexes, typename T, typename ... Types>
struct make_indexes_impl<I, index_tuple<Indexes...>, T, Types...>
{
	typedef typename make_indexes_impl<I+1, index_tuple<Indexes..., I>, Types...>::_ _;
};
template<int I, int... Indexes> struct make_indexes_impl<I, index_tuple<Indexes...>> {typedef index_tuple<Indexes...> _;};
template<typename... Types> struct make_indexes: make_indexes_impl<0, index_tuple<>, Types...> {};
}

// ----------- FOR EACH -----------------
/*template<typename Func, typename Last>
void for_each_impl(Func&& f, Last&& last)
{
	f(last);
}

template<typename Func, typename First, typename... Rest>
void for_each_impl(Func&& f, First&& first, Rest&&...rest)
{
	f(first);
	for_each_impl(core::forward<Func>(f), rest...);
}*/


template<typename ...Args> struct Tuple;
template<> struct Tuple<>
{
	typedef TypeList<> TL;
	enum {ElementCount=0};

	template<typename Func> void ForEachField(Func&& f) const {(void)f;}
	template<typename Func> void TransformEach(const Func& f) const {(void)f;}
};

template<typename H, typename... Args> struct Tuple<H, Args...>
{
	typedef Tuple<Args...> NextTuple;
	typedef TypeList<H, Args...> TL;
	enum {ElementCount = 1+sizeof...(Args)};

	template<typename H1, typename H2, typename... Args1> constexpr Tuple(H1&& h, H2&& h2, Args1&&... args):
		first(core::forward<H1>(h)), next(core::forward<H2>(h2), core::forward<Args1>(args)...) {}

	constexpr Tuple(const Tuple& rhs) = default;

	template<typename H1> constexpr explicit Tuple(H1&& h, Tuple<Args...>&& args):
		first(core::forward<H>(h)), next(core::forward<Tuple<Args...>>(args)) {}

	constexpr Tuple(): first() {}

	bool operator==(const Tuple<H, Args...>& rhs) {return first==rhs.first && next==rhs.next;}
	bool operator!=(const Tuple<H, Args...>& rhs) {return !operator==(rhs);}



	H first;
	NextTuple next;

public:
	template<typename Func> void ForEachField(Func&& f)
	{
		f(first);
		next.ForEachField(core::forward<Func>(f));
	}

	template<typename Func> void ForEachField(Func&& f) const
	{
		f(first);
		next.ForEachField(core::forward<Func>(f));
	}

	template<typename Func> Tuple<decltype(Meta::Val<Func>()(Meta::Val<H>())), decltype(Meta::Val<Func>()(Meta::Val<Args>()))...>
		TransformEach(const Func& f) const
	{
		return Tuple<decltype(Meta::Val<Func>()(Meta::Val<H>())), decltype(Meta::Val<Func>()(Meta::Val<Args>()))...>(f(first), next.TransformEach(f));
	}
};


template<typename H> struct Tuple<H>
{
private:
	typedef RemoveConstRef<H> HNoCR;
public:
	typedef Tuple<> NextTuple;
	typedef TypeList<H> TL;
	enum {ElementCount=1};

	template<typename H1> constexpr Tuple(const Tuple<H1>& h): first(h.first) {}

	constexpr Tuple(HNoCR&& h): first(core::move(h)) {}
	constexpr Tuple(const HNoCR& h): first(h) {}

	constexpr Tuple(const Tuple& rhs) = default;
	//constexpr Tuple(const H& h, const Tuple<>&): first(h) {}
	//constexpr Tuple(H&& h, Tuple<>&&): first(core::forward<H>(h)) {}
	constexpr Tuple(): first() {}
	//~Tuple() {first.~H();}

	bool operator==(const Tuple<H>& rhs) {return first==rhs.first;}
	bool operator!=(const Tuple<H>& rhs) {return !operator==(rhs);}

	H first;

	template<typename Func> void ForEachField(Func&& f) {f(first);}
	template<typename Func> void ForEachField(Func&& f) const {f(first);}
	template<typename Func> Tuple<decltype(Meta::Val<Func>()(Meta::Val<H>()))>
		TransformEach(const Func& f) const {return Tuple<decltype(Meta::Val<Func>()(Meta::Val<H>()))>(f(first));}
};

template<typename... Args> Tuple<Args...> TupleL(Args&&... args) {return Tuple<Args...>(core::forward<Args>(args)...);}


template<typename K, typename V> struct KeyValuePair
{
	typedef TypeList<K, V> TL;

	K Key;
	V Value;

	KeyValuePair(): Key(), Value() {};
	template<typename K1, typename V1> KeyValuePair(K1&& key, V1&& value):
		Key(core::forward<K1>(key)), Value(core::forward<V1>(value)) {}

	operator KeyValuePair<const K, V>() const
	{
		return *reinterpret_cast<KeyValuePair<const K, V>*>(this);
	}

	operator core::pair<K,V>&() {return *reinterpret_cast<core::pair<K,V>*>(this);}
	operator const core::pair<K,V>&() const {return *reinterpret_cast<core::pair<K,V>*>(this);}

	template<typename F> void ForEachField(F&& f) {f(Key); f(Value);}
	template<typename F> void ForEachField(F&& f) const {f(Key); f(Value);}
	template<typename F> KeyValuePair<ResultOf<F, K>, ResultOf<F, V>> TransformEach(F&& f) {return {f(Key), f(Value)};}
};

template<typename T1, typename T2> core::pair<T1, T2> PairL(T1&& first, T2&& second)
{
	return core::pair<T1, T2>(core::forward<T1>(first), core::forward<T2>(second));
}

template<typename K, typename V> KeyValuePair<K, V> KVPairL(K&& key, V&& value)
{
	return KeyValuePair<K, V>(core::forward<K>(key), core::forward<V>(value));
}

template<typename T> struct IsTuple: TypeFromValue<bool, false> {};
template<typename... Args> struct IsTuple<Tuple<Args...>>: TypeFromValue<bool, true> {};
template<typename T1, typename T2> struct IsTuple<core::pair<T1, T2>>: TypeFromValue<bool, true> {};
template<typename K, typename V> struct IsTuple<KeyValuePair<K, V>>: TypeFromValue<bool, true> {};

namespace detail {

template<size_t I> struct Tuple_GetN_T
{
	template<typename T0, typename ...Args> static forceinline auto
		cfunc(const Tuple<T0, Args...>& t) -> decltype(Tuple_GetN_T<I-1>::cfunc(t.next))
	{
		return Tuple_GetN_T<I-1>::cfunc(t.next);
	}

	template<typename T0, typename ...Args> static forceinline auto
		func(Tuple<T0, Args...>& t) -> decltype(Tuple_GetN_T<I-1>::func(t.next))
	{
		return Tuple_GetN_T<I-1>::func(t.next);
	}
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
	template<typename T1, typename T2> static forceinline const T1& get(const core::pair<T1, T2>& p) {return p.first;}
	template<typename T1, typename T2> static forceinline T1& get(core::pair<T1, T2>& p) {return p.first;}

	template<typename K, typename V> static forceinline const K& get(const KeyValuePair<K, V>& p) {return p.Key;}
	template<typename K, typename V> static forceinline K& get(KeyValuePair<K, V>& p) {return p.Key;}
};

template<size_t I> struct Pair_Get_T;
template<> struct Pair_Get_T<1>
{
	template<typename T1, typename T2> static forceinline const T1& get(const core::pair<T1, T2>& p) {return p.second;}
	template<typename T1, typename T2> static forceinline T1& get(core::pair<T1, T2>& p) {return p.second;}

	template<typename K, typename V> static forceinline const V& get(const KeyValuePair<K, V>& p) {return p.Value;}
	template<typename K, typename V> static forceinline V& get(KeyValuePair<K, V>& p) {return p.Value;}
};

}


template<size_t N, typename... Types> forceinline
TypeListAt<N, TypeList<Types...>>& Get(Tuple<Types...>& tuple)
{return detail::Tuple_GetN_T<N>::func(tuple);}

template<size_t N, typename... Types> forceinline
const TypeListAt<N, TypeList<Types...>>& Get(const Tuple<Types...>& tuple)
{return detail::Tuple_GetN_T<N>::cfunc(tuple);}

template<size_t N, typename T1, typename T2> forceinline
Meta::SelectType<T1, T2, N==0>& Get(core::pair<T1, T2>& p)
{return detail::Pair_Get_T<N>::get(p);}

template<size_t N, typename T1, typename T2> forceinline
const Meta::SelectType<T1, T2, N==0>& Get(const core::pair<T1, T2>& p)
{return detail::Pair_Get_T<N>::get(p);}

template<size_t N, typename T1, typename T2> forceinline
Meta::SelectType<T1, T2, N==0>& Get(KeyValuePair<T1, T2>& p)
{return detail::Pair_Get_T<N>::get(p);}

template<size_t N, typename K, typename V> forceinline
const Meta::SelectType<K, V, N==0>& Get(const KeyValuePair<K, V>& p)
{return detail::Pair_Get_T<N>::get(p);}


template<size_t N, typename T> struct TupleElementEqualsFunctor
{
	const T& Value;
	forceinline TupleElementEqualsFunctor(const T& value): Value(value) {}
	template<typename TUPLE> forceinline bool operator()(const TUPLE& rhs) const {return Value==Get<N>(rhs);}
};

template<size_t N, typename T> forceinline TupleElementEqualsFunctor<N, T> TupleElementEquals(const T& value)
{return TupleElementEqualsFunctor<N, T>(value);}


}

using Meta::KeyValuePair;

}

#ifdef _MSC_VER
#pragma warning(pop)
#endif

