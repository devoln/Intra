#pragma once

#include "TList.h"
#include "Functional.h"
#include "Reflection.h"

INTRA_BEGIN
template<typename... Args> struct Tuple;
INTRA_END

namespace std {
//For compatibility with STL and C++17 structured bindings
INTRA_WARNING_PUSH
INTRA_WARNING_DISABLE_STD_DECLARATION
template<class T> struct tuple_size;
INTRA_WARNING_POP
template<typename... Ts> struct tuple_size<Intra::Tuple<Ts...>> {enum: size_t {value = sizeof...(Ts)};};
}

INTRA_BEGIN
INTRA_WARNING_DISABLE_COPY_MOVE_CONSTRUCT_IMPLICITLY_DELETED

INTRA_DEFINE_CONCEPT_REQUIRES(CTupleSizeDefined, std::tuple_size<T>::value);

namespace D {


template<size_t I, typename ValueType,
	bool IsEmpty = CEmptyClass<ValueType> && !CFinalClass<ValueType>
> struct TupleLeaf;

template<size_t I, typename ValueType, bool IsEmpty> struct TupleLeaf
{
	ValueType value;

	TupleLeaf& operator=(const TupleLeaf&) = default;

	TupleLeaf() = default;
	//static_assert(!CReference<ValueType>, "Attempt to default construct a reference element in a tuple");

	TupleLeaf(const TupleLeaf& t) = default;
	//static_assert(!CRValueReference<ValueType>, "Can not copy a tuple with rvalue reference member");

	template<typename T, typename = typename Requires<
		CConstructible<ValueType, T>>
	> constexpr forceinline explicit TupleLeaf(T&& t): value(Forward<T>(t))
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

	template<typename T> constexpr forceinline explicit TupleLeaf(const TupleLeaf& t): value(t.value) {}

	template<typename T> constexpr forceinline TupleLeaf& operator=(T&& t)
	{
		value = Forward<T>(t);
		return *this;
	}

	constexpr forceinline void swap(TupleLeaf& rhs)
	{
		Swap(value, rhs.value);
	}

	constexpr forceinline ValueType& Value() {return value;}
    constexpr const ValueType& Value() const {return value;}
};

template<size_t I, typename ValueType> struct TupleLeaf<I, ValueType, true>: private ValueType
{
private:
	using VT = ValueType; // used by Core.natvis
    TupleLeaf& operator=(const TupleLeaf&) = delete;
public:
	TupleLeaf() = default;
 
	TupleLeaf(const TupleLeaf& rhs) = default;
	//static_assert(!CRValueReference<ValueType_>, "Can not copy a tuple with rvalue reference member");
 
    template <typename T, typename = typename Requires<
        CConstructible<ValueType, T>
	>> constexpr forceinline explicit TupleLeaf(T&& rhs): ValueType(Forward<T>(rhs)) {}
 
    template<typename T> constexpr forceinline explicit TupleLeaf(const TupleLeaf<I, T>& rhs): ValueType(rhs.Value) {}
 
    template<typename T> constexpr forceinline TupleLeaf& operator=(T&& t)
	{
        ValueType::operator=(Forward<T>(t));
        return *this;
    }
 
    constexpr forceinline ValueType& Value() {return static_cast<ValueType&>(*this);}
    constexpr const ValueType& Value() const {return static_cast<const ValueType&>(*this);}
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
struct TupleImpl<TIndexSeq<Indices...>, Ts...>: public TupleLeaf<Indices, Ts>...
{
	using Reflection_FieldTypes = TList<Ts...>;

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

	template<typename OtherTuple> constexpr TupleImpl(OtherTuple&& t):
		TupleLeaf<Indices, Ts>(Forward<
			TListAt<Indices, TMakeTupleTList<OtherTuple>>
		>(get<Indices>(t)))... {}

	template<typename OtherTuple> constexpr TupleImpl& operator=(OtherTuple&& t)
	{
		TExpand{(
			TupleLeaf<Indices, Ts>::operator=(Forward<TListAt<Indices, TMakeTupleTList<OtherTuple>>>(get<Indices>(t))),
		'\0')...};
		return *this;
	}

	constexpr TupleImpl& operator=(const TupleImpl& t)
	{
		TExpand{(
			TupleLeaf<Indices, Ts>::operator=(static_cast<const TupleLeaf<Indices, Ts>&>(t).get()),
		'\0')...};
		return *this;
	}

	constexpr void swap(TupleImpl& t)
	{
		TExpand{(
			TupleLeaf<Indices, Ts>::swap(static_cast<TupleLeaf<Indices, Ts>&>(t)),
		'\0')...};
	}

	template<size_t I> friend INTRA_NODISCARD constexpr forceinline auto&& get(TupleImpl& t)
	{
		static_assert(I < sizeof...(Ts), "Field index is too large for this tuple type!");
		typedef TAtIndex<I, Ts...> Type;
		return static_cast<D::TupleLeaf<I, Type>&>(t).Value();
	}
	template<size_t I> friend INTRA_NODISCARD constexpr forceinline auto&& get(const TupleImpl& t)
	{
		static_assert(I < sizeof...(Ts), "Field index is too large for this tuple type!");
		typedef TAtIndex<I, Ts...> Type;
		return static_cast<const D::TupleLeaf<I, Type>&>(t).Value();
	}
	template<size_t I> friend INTRA_NODISCARD constexpr forceinline auto&& get(TupleImpl&& t)
	{
		static_assert(I < sizeof...(Ts), "Field index is too large for this tuple type!");
		typedef TAtIndex<I, Ts...> Type;
		return static_cast<Type&&>(static_cast<D::TupleLeaf<I, Type>&&>(t).Value());
	}
};

}

template<class From, class To> concept CTListConvertible = CListAllPairs<CConvertibleT, From, To>;
template<class To, class From> concept CTListAssignable = CListAllPairs<CAssignableT, To, From>;


INTRA_DEFINE_CONCEPT_REQUIRES(CHasReflection_FieldTypes, T::ReflectionFieldTypes);

namespace D {
template<class From, class To, bool = CHasReflection_FieldTypes<From> && CHasReflection_FieldTypes<To>> struct CTupleConvertibleT {enum: bool {_ = false};};
template<class From, class To> struct CTupleConvertibleT<From, To, true> {enum: bool {_ = CTListConvertible<D::TMakeTupleTList<From>, D::TMakeTupleTList<To>>};};
template<class To, class From, bool = CHasReflection_FieldTypes<From> && CHasReflection_FieldTypes<To>> struct CTListAssignableT {enum: bool {_ = false};};
template<class To, class From> struct CTListAssignableT<To, From, true> {enum: bool {_ = CTListAssignable<D::TMakeTupleTList<To>, D::TMakeTupleTList<From>>};};
}
template<class From, class To> concept CTupleConvertible = D::CTupleConvertibleT<From, To>::_;
template<class To, class From> concept CTupleAssignable = D::CTListAssignableT<To, From>::_;


template<typename... Ts> struct Tuple
{
private:
	template<index_t I> using Leaf = D::TupleLeaf<I, TAtIndex<I, Ts...>>; //used by Core.natvis
	typedef D::TupleImpl<TMakeIndexSeq<sizeof...(Ts)>, Ts...> Impl;
	Impl impl_;
public:
	using Reflection_FieldTypes = TList<Ts...>;

	//TODO: must be explicit if !VAll(CConvertible<const Ts&, Ts>...)
	constexpr /*explicit*/ Tuple(const Ts&... t): impl_(
		TMakeIndexSeq<sizeof...(Ts)>(),
		D::TMakeTupleTListSlice<Tuple, sizeof...(Ts)>(),
		TMakeIndexSeq<0>(),
		D::TMakeTupleTListSlice<Tuple, 0>(),
		t...) {}

	template <typename... Us, typename = Requires<
		sizeof...(Us) <= sizeof...(Ts) &&
		CTListConvertible<TList<Us...>, TListSlice<TList<Ts...>, 0, FMin(sizeof...(Us), sizeof...(Ts))>>
	>> constexpr Tuple(Us&&... u)
		: impl_(TMakeIndexSeq<sizeof...(Us)>(),
			TListSlice<TList<Ts...>, 0, sizeof...(Us)>(),
			TIndexRange<sizeof...(Ts), sizeof...(Us)>(),
			TListSlice<TList<Ts...>, sizeof...(Us), sizeof...(Ts)>(),
			Forward<Us>(u)...) {}

	template <typename OtherTuple, typename = Requires<
		CTupleConvertible<OtherTuple, Tuple>
	>> constexpr forceinline Tuple(OtherTuple&& t): impl_(Forward<OtherTuple>(t)) {}

	template <typename OtherTuple, typename = Requires<
		CTupleAssignable<Tuple, OtherTuple>
	>> constexpr forceinline Tuple& operator=(OtherTuple&& t)
	{
		impl_.operator=(Forward<OtherTuple>(t));
		return *this;
	}

	constexpr forceinline void swap(Tuple& t) {impl_.swap(t.impl_);}

	template<size_t I> INTRA_NODISCARD constexpr forceinline decltype(auto) Field() & {return get<I>(impl_);}
	template<size_t I> INTRA_NODISCARD constexpr forceinline decltype(auto) Field() const& {return get<I>(impl_);}
	template<size_t I> INTRA_NODISCARD constexpr forceinline decltype(auto) Field() && {return Move(get<I>(Move(impl_)));}

	//We are not using CamelCase as an exception for compatibility with STL and C++17 structured bindings
	template<size_t I> friend INTRA_NODISCARD constexpr forceinline decltype(auto) get(Tuple& t) {return get<I>(t.impl_);}
	template<size_t I> friend INTRA_NODISCARD constexpr forceinline decltype(auto) get(const Tuple& t) {return get<I>(t.impl_);}
	template<size_t I> friend INTRA_NODISCARD constexpr forceinline decltype(auto) get(Tuple&& t) {return Move(get<I>(Move(t.impl_)));}
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
	>> constexpr forceinline Tuple& operator=(OtherTuple&&) {return *this;}

	constexpr forceinline void swap(Tuple&) {}
};

#if defined(__cpp_deduction_guides) && __cpp_deduction_guides >= 201703
template<class... Ts> Tuple(Ts...) -> Tuple<Ts...>;
#endif

namespace D {
template<typename T, typename F, size_t... Is> constexpr void ForEachField_(T&& t, F& f, TIndexSeq<Is...>)
{
#if defined(__cpp_fold_expressions) && __cpp_fold_expressions >= 201603
	(void(f(get<Is>(t))), ...);
#else
	TExpand{(char((f(get<Is>(t)), '\0')))...};
#endif
}
template<typename T, typename F, size_t... Is> constexpr void ForEachFieldIndex_(T&& t, F& f, TIndexSeq<Is...>)
{
#if defined(__cpp_fold_expressions) && __cpp_fold_expressions >= 201603
	(void(f(t, Is)), ...);
#else
	TExpand{(char((f(t, Is), '\0')))...};
#endif
}
template<typename T, typename F, size_t... Is> constexpr auto TransformEachField_(T&& t, F& f, TIndexSeq<Is...>)
{return Tuple<decltype(f(get<Is>(t)))...>{f(get<Is>(t))...};}
}

template<typename T, typename F> constexpr forceinline Requires<
	CTupleSizeDefined<TRemoveConstRef<T>>
> ForEachField(T&& t, F&& f)
{D::ForEachField_(Forward<T>(t), f, TMakeIndexSeq<std::tuple_size<TRemoveConstRef<T>>::value>());}

template<typename T, typename F> constexpr forceinline Requires<
	CTupleSizeDefined<T>
> ForEachFieldIndex(const T& t, F&& f)
{D::ForEachFieldIndex_(t, f, TMakeIndexSeq<std::tuple_size<T>::value>());}

template<typename T, typename F, Requires<
	CTupleSizeDefined<T>
>* = null> constexpr forceinline auto TransformEachField(const T& t, F&& f)
{return D::TransformEachField_(t, f, TMakeIndexSeq<std::tuple_size<T>::value>());}

namespace D {
template<typename Tuple1, typename Tuple2, size_t... I>
constexpr bool TupleEqual(const Tuple1& lhs, const Tuple2& rhs, TIndexSeq<I...>)
{
	bool result = true;
	TExpand{(result = result && get<I>(lhs) == get<I>(rhs))...};
	return result;
}
template<typename Tuple1, typename Tuple2, size_t... I>
constexpr int TupleCompare(const Tuple1& lhs, const Tuple2& rhs, TIndexSeq<I...>)
{
	sbyte result = 0;
	TExpand{(result = result == -1? -1: result == 1? 1: 
		get<I>(lhs) < get<I>(rhs)? -1:
		get<I>(rhs) < get<I>(lhs)? 1:
		0)...};
	return result;
}
}

template<typename... Ts1, typename... Ts2>
INTRA_NODISCARD constexpr forceinline bool operator==(const Tuple<Ts1...>& lhs, const Tuple<Ts2...>& rhs)
{return D::TupleEqual(lhs, rhs, TSeqFor<Ts1...>{});}

template<typename... Ts1, typename... Ts2>
INTRA_NODISCARD constexpr forceinline bool operator!=(const Tuple<Ts1...>& lhs, const Tuple<Ts2...>& rhs) {return !(lhs == rhs);}

template<typename... Ts1, typename... Ts2>
INTRA_NODISCARD constexpr forceinline bool operator<(const Tuple<Ts1...>& lhs, const Tuple<Ts2...>& rhs)
{return D::TupleCompare(lhs, rhs, TSeqFor<Ts1...>{}) < 0;}

template<typename... Ts1, typename... Ts2>
INTRA_NODISCARD constexpr forceinline bool operator>(const Tuple<Ts1...>& lhs, const Tuple<Ts2...>& rhs) {return rhs < lhs;}

template<typename... Ts1, typename... Ts2>
INTRA_NODISCARD constexpr forceinline bool operator<=(const Tuple<Ts1...>& lhs, const Tuple<Ts2...>& rhs) {return !(rhs < lhs);}

template<typename... Ts1, typename... Ts2>
INTRA_NODISCARD constexpr forceinline bool operator>=(const Tuple<Ts1...>& lhs, const Tuple<Ts2...>& rhs) {return !(lhs < rhs);}

template<typename... Ts> INTRA_NODISCARD constexpr forceinline Tuple<Ts...> TupleL(Ts&&... args) {return {Forward<Ts>(args)...};}

template<typename... Ts> constexpr forceinline auto Tie(Ts& ... args) {return Tuple<Ts&...>(args...);}

namespace D {
template <class F, class Tuple, size_t... I>
constexpr decltype(auto) apply_impl(F&& f, Tuple&& t, TIndexSeq<I...>)
{
	return Forward<F>(f)(get<I>(Forward<Tuple>(t))...);
}
}

template<typename F, typename Tuple> constexpr decltype(auto) Apply(F&& f, Tuple&& t)
{
	return D::apply_impl(
		Forward<F>(f), Forward<Tuple>(t),
		TMakeIndexSeq<std::tuple_size<TRemoveConstRef<Tuple>>::value>{});
}

template<size_t N, typename T> struct TupleElementEqualsFunctor
{
	const T& Value;
	template<typename TUPLE> INTRA_NODISCARD constexpr forceinline bool operator()(const TUPLE& rhs) const {return Value == get<N>(rhs);}
};

template<size_t N, typename T> INTRA_NODISCARD constexpr forceinline TupleElementEqualsFunctor<N, T> TupleElementEquals(const T& value)
{return TupleElementEqualsFunctor<N, T>{value};}

//TODO: equivalent to https://en.cppreference.com/w/cpp/utility/make_from_tuple

INTRA_END
