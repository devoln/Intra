#pragma once

#include "TList.h"
#include "Functional.h"

INTRA_BEGIN
INTRA_WARNING_DISABLE_COPY_MOVE_CONSTRUCT_IMPLICITLY_DELETED
inline namespace Core {

template<typename... Args> struct Tuple;

namespace D {

template<size_t I, typename ValueType,
	bool IsEmpty = CEmptyClass<ValueType> && !CFinalClass<ValueType>
> struct TupleLeaf;

template<size_t I, typename ValueType, bool IsEmpty> struct TupleLeaf
{
	ValueType Value;

	TupleLeaf& operator=(const TupleLeaf&) = default;

	TupleLeaf() = default;
	//static_assert(!CReference<ValueType>, "Attempt to default construct a reference element in a tuple");

	TupleLeaf(const TupleLeaf& t) = default;
	//static_assert(!CRValueReference<ValueType>, "Can not copy a tuple with rvalue reference member");

	template<typename T, typename = typename Requires<
		CConstructible<ValueType, T>>
	> constexpr forceinline explicit TupleLeaf(T&& t): Value(Forward<T>(t))
	{
		static_assert(!CReference<ValueType> ||
			(
				CLValueReference<ValueType> &&
				(
					CLValueReference<T>/* ||
					CSame<TRemoveReference<T>,
						reference_wrapper<TRemoveReference<ValueType_>>>*/
				)
			) || (CRValueReference<ValueType> && !CLValueReference<T>),
			"Attempted to construct a reference element in a tuple with an rvalue");
	}

	template<typename T> constexpr forceinline explicit TupleLeaf(const TupleLeaf<I, T>& t): Value(t.Value) {}

	template<typename T> INTRA_CONSTEXPR2 forceinline TupleLeaf& operator=(T&& t)
	{
		Value = Forward<T>(t);
		return *this;
	}

	INTRA_CONSTEXPR2 forceinline void swap(TupleLeaf& rhs)
	{
		Swap(Value, rhs.Value);
	}

	INTRA_CONSTEXPR2 forceinline ValueType& get() {return Value;}
    constexpr forceinline const ValueType& get() const {return Value;}
};

template<size_t I, typename ValueType> struct TupleLeaf<I, ValueType, true>: private ValueType {
 
    TupleLeaf& operator=(const TupleLeaf&) = delete;
public:
	TupleLeaf() = default;
 
	TupleLeaf(const TupleLeaf& rhs) = default;
	//static_assert(!CRValueReference<ValueType_>, "Can not copy a tuple with rvalue reference member");
 
    template <typename T, typename = typename Requires<
        CConstructible<ValueType, T>
	>> constexpr forceinline explicit TupleLeaf(T&& rhs): ValueType(Forward<T>(rhs)) {}
 
    template<typename T> constexpr forceinline explicit TupleLeaf(const TupleLeaf<I, T>& rhs): ValueType(rhs.Value) {}
 
    template<typename T> INTRA_CONSTEXPR2 forceinline TupleLeaf& operator=(T&& t)
	{
        ValueType::operator=(Forward<T>(t));
        return *this;
    }
 
    INTRA_CONSTEXPR2 forceinline ValueType& get() {return static_cast<ValueType&>(*this);}
    constexpr const ValueType& get() const {return static_cast<const ValueType&>(*this);}
};

template<size_t I, typename T> struct TupleElement;//: public std::tuple_element<I, T> {};
template<size_t I, typename... Ts> struct TupleElement<I, Tuple<Ts...>> {typedef TAtIndex<I, Ts...> _;};
template<size_t I, typename... Ts> struct TupleElement<I, const Tuple<Ts...>> {typedef const TAtIndex<I, Ts...> _;};
template<size_t I, typename... Ts> struct TupleElement<I, volatile Tuple<Ts...>> {typedef volatile TAtIndex<I, Ts...> _;};
template<size_t I, typename... Ts> struct TupleElement<I, const volatile Tuple<Ts...>> {typedef const volatile TAtIndex<I, Ts...> _;};

template<typename T> using TFieldTList = typename TRemoveReference<T>::Reflection_FieldTypes;

template<typename Tuple>
using TMakeTupleTList = TListTransform1<
	TFieldTList<Tuple>,
	TPropagateQualLVRef, Tuple
>;

template<typename Tuple, size_t End = TFieldTList<Tuple>::Length, size_t Start = 0>
using TMakeTupleTListSlice = TListTransform1<
	TListSlice<TFieldTList<Tuple>, Start, End>,
	TPropagateQualLVRef, Tuple
>;


template <typename TupleIndexes_, typename... Ts_>
struct TupleImpl;

template<size_t... Indices, typename... Ts>
struct TupleImpl<TIndexSeq<Indices...>, Ts...>
	: public TupleLeaf<Indices, Ts>... {
	template <size_t... FirstIndices, typename... FirstTypes,
		size_t... LastIndices, typename... LastTypes,
		typename... ValueTypes
	> constexpr explicit TupleImpl(
		TIndexSeq<FirstIndices...>, TList<FirstTypes...>,
		TIndexSeq<LastIndices...>, TList<LastTypes...>,
		ValueTypes&& ... values
	): TupleLeaf<FirstIndices, FirstTypes>(Forward<ValueTypes>(values))...,
		TupleLeaf<LastIndices, LastTypes>()...
	{}

	template<typename OtherTuple>
	constexpr TupleImpl(OtherTuple&& t):
		TupleLeaf<Indices, Ts>(Forward<
			TListAt<Indices, TMakeTupleTList<OtherTuple>>
		>(GetField<Indices>(t)))... {}

	template<typename OtherTuple>
	INTRA_CONSTEXPR2 TupleImpl& operator=(OtherTuple&& t)
	{
		TExpand{(
			TupleLeaf<Indices, Ts>::operator=(Forward<TListAt<Indices, TMakeTupleTList<OtherTuple>>>(GetField<Indices>(t))),
		'\0')...};
		return *this;
	}

	INTRA_CONSTEXPR2 TupleImpl& operator=(const TupleImpl& t)
	{
		TExpand{(
			TupleLeaf<Indices, Ts>::operator=(static_cast<const TupleLeaf<Indices, Ts>&>(t).get()),
		'\0')...};
		return *this;
	}

	INTRA_CONSTEXPR2 void swap(TupleImpl& t)
	{
		TExpand{(
			TupleLeaf<Indices, Ts>::swap(static_cast<TupleLeaf<Indices, Ts>&>(t)),
		'\0')...};
	}
};

}

template<class From, class To> concept CTListConvertible = CListAllPairs<CConvertibleT, From, To>;
template<class To, class From> concept CTListAssignable = CListAllPairs<CAssignableT, To, From>;
template<class From, class To> concept CTupleConvertible = CTListConvertible<D::TMakeTupleTList<From>, D::TMakeTupleTList<To>>;
template<class To, class From> concept CTupleAssignable = CTListAssignable<D::TMakeTupleTList<To>, D::TMakeTupleTList<From>>;


template<typename... Ts> struct Tuple
{
private:
	typedef D::TupleImpl<TMakeIndexSeq<sizeof...(Ts)>, Ts...> Impl;
	Impl impl_;

	template<size_t I, typename... Ts> friend constexpr decltype(auto) GetField(Tuple<Ts...>& t);
	template<size_t I, typename... Ts> friend constexpr decltype(auto) GetField(const Tuple<Ts...>& t);
	template<size_t I, typename... Ts> friend constexpr decltype(auto) GetField(Tuple<Ts...>&& t);
public:
	typedef TList<Ts...> Reflection_FieldTypes;

	constexpr explicit Tuple(const Ts&... t): impl_(
		TMakeIndexSeq<sizeof...(Ts)>(),
		D::TMakeTupleTListSlice<Tuple, sizeof...(Ts)>(),
		TMakeIndexSeq<0>(),
		D::TMakeTupleTListSlice<Tuple, 0>(),
		t...) {}

	template <typename... Us, typename = Requires<
		sizeof...(Us) <= sizeof...(Ts) &&
		CTListConvertible<TList<Us...>, TListSlice<TList<Ts...>, (sizeof...(Us) < sizeof...(Ts))? sizeof...(Us): sizeof...(Ts)>>
	>> constexpr explicit Tuple(Us&&... u)
		: impl_(TMakeIndexSeq<sizeof...(Us)>(),
			TListSlice<TList<Ts...>, sizeof...(Us)>(),
			TIndexRange<sizeof...(Ts), sizeof...(Us)>(),
			TListSlice<TList<Ts...>, sizeof...(Ts), sizeof...(Us)>(),
			Forward<Us>(u)...) {}

	template <typename OtherTuple, typename = Requires<
		CTupleConvertible<OtherTuple, Tuple>
	>> constexpr forceinline Tuple(OtherTuple&& t): impl_(Forward<OtherTuple>(t)) {}

	template <typename OtherTuple, typename = Requires<
		CTupleAssignable<Tuple, OtherTuple>
	>> INTRA_CONSTEXPR2 forceinline Tuple& operator=(OtherTuple&& t)
	{
		impl_.operator=(Forward<OtherTuple>(t));
		return *this;
	}

	INTRA_CONSTEXPR2 forceinline void swap(Tuple& t) {impl_.swap(t.impl_);}


	template<size_t I> constexpr forceinline decltype(auto) Field() &
	{
		typedef TAtIndex<I, Ts...> Type;
		return static_cast<D::TupleLeaf<I, Type>&>(impl_).get();
	}

	template<size_t I> constexpr forceinline decltype(auto) Field() const&
	{
		typedef TAtIndex<I, Ts...> Type;
		return static_cast<const D::TupleLeaf<I, Type>&>(impl_).get();
	}

	template<size_t I> constexpr forceinline decltype(auto) Field() &&
	{
		typedef TAtIndex<I, Ts...> Type;
		return static_cast<Type&&>(static_cast<D::TupleLeaf<I, Type>&&>(impl_).get());
	}
};

template<> struct Tuple<>
{
	typedef TList<> Reflection_FieldTypes;

	Tuple() = default;
	Tuple(Tuple&&) = default;
	Tuple(const Tuple&) = default;
	Tuple& operator=(const Tuple&) = default;
	Tuple& operator=(Tuple&&) = default;

	template<typename OtherTuple, typename = Requires<
		CTupleConvertible<OtherTuple, Tuple>
	>> constexpr forceinline Tuple(OtherTuple&&) {}

	template<typename OtherTuple, typename Requires<
		CTupleAssignable<Tuple, OtherTuple>
	>> INTRA_CONSTEXPR2 forceinline Tuple& operator=(OtherTuple&&) {return *this;}

	INTRA_CONSTEXPR2 forceinline void swap(Tuple&) {}
};

#if defined(__cpp_deduction_guides) && __cpp_deduction_guides >= 201703
template<class... Ts> Tuple(Ts...) -> Tuple<Ts...>;
#endif

template<size_t I, typename... Ts> INTRA_NODISCARD constexpr forceinline decltype(auto) GetField(Tuple<Ts...>& t) {return t.template Field<N>();}
template<size_t I, typename... Ts> INTRA_NODISCARD constexpr forceinline decltype(auto) GetField(const Tuple<Ts...>& t) {return t.template Field<N>();}
template<size_t I, typename... Ts> INTRA_NODISCARD constexpr forceinline decltype(auto) GetField(Tuple<Ts...>&& t) {return Move(t.template Field<N>());}

//For compatibility with STL and C++17 structured bindings
template<size_t I, typename... Ts> INTRA_NODISCARD constexpr forceinline decltype(auto) get(Tuple<Ts...>& t) {return t.template Field<N>();}
template<size_t I, typename... Ts> INTRA_NODISCARD constexpr forceinline decltype(auto) get(const Tuple<Ts...>& t) {return t.template Field<N>();}
template<size_t I, typename... Ts> INTRA_NODISCARD constexpr forceinline decltype(auto) get(Tuple<Ts...>&& t) {return Move(t.template Field<N>());}

namespace D {
template<typename T, typename F, size_t... Is> INTRA_CONSTEXPR2 void ForEachField_(T&& t, F& f, TIndexSeq<Is...>)
{
#if defined(__cpp_fold_expressions) && __cpp_fold_expressions >= 201603
	(void(f(GetField<Is>(t))), ...);
#else
	TExpand{(void(f(GetField<Is>(t)), '\0'))...};
#endif
}
template<typename T, typename F, size_t... Is> INTRA_CONSTEXPR2 void ForEachFieldIndex_(T&& t, F& f, TIndexSeq<Is...>)
{
#if defined(__cpp_fold_expressions) && __cpp_fold_expressions >= 201603
	(void(f(t, Is)), ...);
#else
	TExpand{(void(f(t, Is), '\0'))...};
#endif
}
template<typename T, typename F, size_t... Is> constexpr auto TransformEachField_(T&& t, F& f, TIndexSeq<Is...>)
{return Tuple<decltype(f(GetField<Is>(t)))...>{f(GetField<Is>(t))...};}
}

template<typename... Ts, typename F> INTRA_CONSTEXPR2 forceinline void ForEachField(const Tuple<Ts...>& t, F&& f)
{D::ForEachField_(t, f, TSeqFor<Ts...>());}

template<typename... Ts, typename F> INTRA_CONSTEXPR2 forceinline void ForEachFieldIndex(const Tuple<Ts...>& t, F&& f)
{D::ForEachFieldIndex_(t, f, TSeqFor<Ts...>());}

template<typename... Ts, typename F> INTRA_CONSTEXPR2 forceinline void AllFields(const Tuple<Ts...>& t, F&& f)
{D::ForEachField_(t, f, TSeqFor<Ts...>());}

template<typename... Ts, typename F> constexpr forceinline auto TransformEachField(const Tuple<Ts...>& t, F&& f)
{return D::TransformEachField_(t, f, TSeqFor<Ts...>());}

namespace D {
template<typename Tuple1, typename Tuple2, size_t... I>
INTRA_CONSTEXPR2 bool TupleEqual(const Tuple1& lhs, const Tuple2& rhs)
{
	bool result = true;
	TExpand{(result = result && GetField<I>(lhs) == GetField<I>(rhs))...};
	return result;
}
template<typename Tuple1, typename Tuple2, size_t... I>
INTRA_CONSTEXPR2 int TupleCompare(const Tuple1& lhs, const Tuple2& rhs)
{
	sbyte result = 0;
	TExpand{(result = result == -1? -1: result == 1? 1: 
		GetField<I>(lhs) < GetField<I>(rhs)? -1:
		GetField<I>(rhs) < GetField<I>(lhs)? 1:
		0)...};
	return result;
}
}

template<typename... Ts1, typename... Ts2>
INTRA_NODISCARD INTRA_CONSTEXPR2 forceinline bool operator==(const Tuple<Ts1...>& lhs, const Tuple<Ts2...>& rhs)
{return D::TupleEqual<TSeqFor<Ts1...>>(lhs, rhs);}

template<typename... Ts1, typename... Ts2>
INTRA_NODISCARD INTRA_CONSTEXPR2 forceinline bool operator!=(const Tuple<Ts1...>& lhs, const Tuple<Ts2...>& rhs) {return !(lhs == rhs);}

template<typename... Ts1, typename... Ts2>
INTRA_NODISCARD INTRA_CONSTEXPR2 forceinline bool operator<(const Tuple<Ts1...>& lhs, const Tuple<Ts2...>& rhs)
{return D::TupleCompare<sizeof...(Ts1)>(lhs, rhs) < 0;}

template<typename... Ts1, typename... Ts2>
INTRA_NODISCARD INTRA_CONSTEXPR2 forceinline bool operator>(const Tuple<Ts1...>& lhs, const Tuple<Ts2...>& rhs) {return rhs < lhs;}

template<typename... Ts1, typename... Ts2>
INTRA_NODISCARD INTRA_CONSTEXPR2 forceinline bool operator<=(const Tuple<Ts1...>& lhs, const Tuple<Ts2...>& rhs) {return !(rhs < lhs);}

template<typename... Ts1, typename... Ts2>
INTRA_NODISCARD INTRA_CONSTEXPR2 forceinline bool operator>=(const Tuple<Ts1...>& lhs, const Tuple<Ts2...>& rhs) {return !(lhs < rhs);}

template<typename... Ts> INTRA_NODISCARD INTRA_CONSTEXPR2 forceinline Tuple<Ts...> TupleL(Ts&&... args) {return {Forward<Ts>(args)...};}

template<typename... Ts> constexpr forceinline auto Tie(Ts& ... args) {return Tuple<Ts&...>(args...);}

namespace D {
template <class F, class Tuple, size_t... I>
constexpr decltype(auto) apply_impl(F&& f, Tuple&& t, TIndexSeq<I...>)
{
	return Forward<F>(f)(GetField<I>(Forward<Tuple>(t))...);
}
}

template<typename F, typename Tuple> constexpr decltype(auto) Apply(F&& f, Tuple&& t)
{
	return D::apply_impl(
		Forward<F>(f), Forward<Tuple>(t),
		TMakeIndexSeq<TRemoveReference<Tuple>::Reflection_NumFields>{});
}

template<size_t N, typename T> struct TupleElementEqualsFunctor
{
	const T& Value;
	template<typename TUPLE> INTRA_NODISCARD constexpr forceinline bool operator()(const TUPLE& rhs) const {return Value == GetField<N>(rhs);}
};

template<size_t N, typename T> INTRA_NODISCARD constexpr forceinline TupleElementEqualsFunctor<N, T> TupleElementEquals(const T& value)
{return TupleElementEqualsFunctor<N, T>{value};}

//TODO: equivalent to https://en.cppreference.com/w/cpp/utility/make_from_tuple

}
INTRA_END

namespace std {
//For compatibility with STL and C++17 structured bindings
INTRA_WARNING_PUSH
INTRA_DISABLE_WARNING_STD_DECLARATION
template<class T> class tuple_size;
INTRA_WARNING_POP
template<typename... Ts> class tuple_size<Intra::Core::Tuple<Ts...>> {enum: size_t {value = sizeof...(Ts)};};
}
