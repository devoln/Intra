#pragma once

#include "Intra/Functional.h"
#include "Intra/Concepts.h"
#include "Intra/TypeSafe.h"
#include "Intra/Operations.h"

INTRA_BEGIN
INTRA_IGNORE_WARNING_COPY_MOVE_CONSTRUCT_IMPLICITLY_DELETED

namespace z_D {
template<size_t I, typename ValueType, bool = CEmptyClass<ValueType> && !CFinalClass<ValueType>>
struct TupleLeaf: private ValueType
{
private:
	using VT = ValueType; // used by MSVC Native Visualizer (natvis)
    TupleLeaf& operator=(const TupleLeaf&) = delete;
protected:
	TupleLeaf() = default;
 
	TupleLeaf(const TupleLeaf& rhs) = default;
	//static_assert(!CRValueReference<ValueType_>, "Can not copy a tuple with rvalue reference member");
 
    template <typename T, typename = Requires<CConstructible<ValueType, T>>>
	constexpr explicit TupleLeaf(T&& rhs): ValueType(Forward<T>(rhs)) {}
 
    template<typename T> constexpr explicit TupleLeaf(const TupleLeaf<I, T>& rhs): ValueType(rhs.Value) {}
 
    template<typename T> constexpr void assign(T&& t) {ValueType::operator=(Forward<T>(t));}
 
    constexpr ValueType& Value() {return static_cast<ValueType&>(*this);}
    constexpr const ValueType& Value() const {return static_cast<const ValueType&>(*this);}
};

template<size_t I, typename ValueType> struct TupleLeaf<I, ValueType, false>
{
protected:
	ValueType value;

	TupleLeaf() = default;
	//static_assert(!CReference<ValueType>, "Attempt to default construct a reference element in a tuple");

	TupleLeaf(const TupleLeaf& t) = default;
	//static_assert(!CRValueReference<ValueType>, "Can not copy a tuple with rvalue reference member");

	template<typename T, typename = Requires<CConstructible<ValueType, T>>>
	constexpr explicit TupleLeaf(T&& t): value(Forward<T>(t))
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

	template<typename T> constexpr explicit TupleLeaf(const TupleLeaf& t): value(t.value) {}

	template<typename T> constexpr void assign(T&& t) {value = Forward<T>(t);}

	constexpr ValueType& Value() {return value;}
    constexpr const ValueType& Value() const {return value;}
};


template<typename IndicesSeq, typename... Ts> struct TupleImpl;

template<size_t... Indices, typename... Ts>
struct TupleImpl<TIndexSeq<Indices...>, Ts...>: TupleLeaf<Indices, Ts>...
{
	template<size_t... FirstIndices, typename... FirstTypes,
		size_t... LastIndices, typename... LastTypes,
		typename... ValueTypes>
	constexpr explicit TupleImpl(
		TIndexSeq<FirstIndices...>, TList<FirstTypes...>,
		TIndexSeq<LastIndices...>, TList<LastTypes...>,
		ValueTypes&&... values):
		TupleLeaf<FirstIndices, FirstTypes>(Forward<ValueTypes>(values))...,
		TupleLeaf<LastIndices, LastTypes>()... {}

	template<typename OtherTuple> constexpr TupleImpl(OtherTuple&& t):
		TupleLeaf<Indices, Ts>(Forward<TListAt<Indices, TQualRefFieldTList<OtherTuple>>>(get<Indices>(t)))... {}

	template<typename OtherTuple> constexpr TupleImpl& operator=(OtherTuple&& t)
	{
		(TupleLeaf<Indices, Ts>::assign(Forward<TListAt<Indices, TQualRefFieldTList<OtherTuple>>>(get<Indices>(t))))...;
		return *this;
	}

	constexpr TupleImpl& operator=(const TupleImpl& t)
	{
		(TupleLeaf<Indices, Ts>::assign(static_cast<const TupleLeaf<Indices, Ts>&>(t).get()))...;
		return *this;
	}

	template<index_t I> [[nodiscard]] friend constexpr auto&& get(TupleImpl& t)
	{
		static_assert(I < sizeof...(Ts), "Field index is too large for this tuple type!");
		using Type = TPackAt<I, Ts...>;
		return static_cast<z_D::TupleLeaf<I, Type>&>(t).Value();
	}
	template<size_t I> [[nodiscard]] friend constexpr auto&& get(const TupleImpl& t)
	{
		static_assert(I < sizeof...(Ts), "Field index is too large for this tuple type!");
		using Type = TPackAt<I, Ts...>;
		return static_cast<const z_D::TupleLeaf<I, Type>&>(t).Value();
	}
	template<size_t I> [[nodiscard]] friend constexpr auto&& get(TupleImpl&& t)
	{
		static_assert(I < sizeof...(Ts), "Field index is too large for this tuple type!");
		using Type = TPackAt<I, Ts...>;
		return static_cast<Type&&>(static_cast<z_D::TupleLeaf<I, Type>&&>(t).Value());
	}
};

}


template<typename... Ts> struct Tuple
{
private:
	template<index_t I> using Leaf = z_D::TupleLeaf<I, TPackAt<I, Ts...>>; //used by MSVC Native Visualizer (natvis)
	using Impl = z_D::TupleImpl<TMakeIndexSeq<sizeof...(Ts)>, Ts...>;
	Impl impl_;
public:
	template<typename UTuple = Tuple, typename = Requires<(CConvertibleTo<const Ts&, Ts> && ...)>>
	constexpr Tuple(const Ts&... t): impl_(
		TMakeIndexSeq<sizeof...(Ts)>(),
		TQualRefFieldTListSlice<UTuple, sizeof...(Ts)>(),
		TMakeIndexSeq<0>(),
		TQualRefFieldTListSlice<Tuple, 0>(),
		t...) {}

	template<typename UTuple = Tuple, typename = void, typename = Requires<(CConvertibleTo<const Ts&, Ts> && ...)>>
    explicit constexpr Tuple(const Ts&... t): impl_(
		TMakeIndexSeq<sizeof...(Ts)>(),
		TQualRefFieldTListSlice<UTuple, sizeof...(Ts)>(),
		TMakeIndexSeq<0>(),
		TQualRefFieldTListSlice<Tuple, 0>(),
		t...) {}

	template<typename... Us, typename = Requires<sizeof...(Us) <= sizeof...(Ts) &&
		CTListConvertible<TList<Us...>, TListSlice<TList<Ts...>, 0, FMin(sizeof...(Us), sizeof...(Ts))>>>>
	constexpr Tuple(Us&&... u): impl_(TMakeIndexSeq<sizeof...(Us)>(),
			TListSlice<TList<Ts...>, 0, sizeof...(Us)>(),
			TIndexRange<sizeof...(Ts), sizeof...(Us)>(),
			TListSlice<TList<Ts...>, sizeof...(Us), sizeof...(Ts)>(),
			Forward<Us>(u)...) {}

	template<class OtherTuple,
	    class = Requires<CTListConvertible<TQualRefFieldTList<OtherTuple>, TQualRefFieldTList<Tuple>>>>
	constexpr Tuple(OtherTuple&& t): impl_(Forward<OtherTuple>(t)) {}

	template<class OtherTuple,
	    class = Requires<CTListAssignable<TQualRefFieldTList<Tuple>, TQualRefFieldTList<OtherTuple>>>>
	constexpr Tuple& operator=(OtherTuple&& t)
	{
		impl_.operator=(Forward<OtherTuple>(t));
		return *this;
	}

	template<size_t I> [[nodiscard]] constexpr decltype(auto) Field() & {return get<I>(impl_);}
	template<size_t I> [[nodiscard]] constexpr decltype(auto) Field() const& {return get<I>(impl_);}
	template<size_t I> [[nodiscard]] constexpr decltype(auto) Field() && {return Move(get<I>(Move(impl_)));}

	//We are not using CamelCase as an exception for compatibility with STL and structured bindings
	template<size_t I> [[nodiscard]] friend constexpr decltype(auto) get(Tuple& t) {return get<I>(t.impl_);}
	template<size_t I> [[nodiscard]] friend constexpr decltype(auto) get(const Tuple& t) {return get<I>(t.impl_);}
	template<size_t I> [[nodiscard]] friend constexpr decltype(auto) get(Tuple&& t) {return Move(get<I>(Move(t.impl_)));}
};
template<> struct Tuple<>
{
	Tuple() = default;
	Tuple(Tuple&&) = default;
	Tuple(const Tuple&) = default;
	Tuple& operator=(const Tuple&) = default;
	Tuple& operator=(Tuple&&) = default;

	template<class OtherTuple, class = Requires<StaticLength<OtherTuple> == 0>>
	constexpr Tuple(OtherTuple&&) {}

	template<class OtherTuple, class = Requires<StaticLength<OtherTuple> == 0>>
	constexpr Tuple& operator=(OtherTuple&&) {return *this;}
};
template<typename... Ts> Tuple(Ts...) -> Tuple<Ts...>;

namespace z_D {
template<typename T, typename F, size_t... Is>
constexpr void ForEachField_(T&& t, F& f, TIndexSeq<Is...>) {(f(get<Is>(t)), ...);}
template<typename T, typename F, size_t... Is>
constexpr void ForEachFieldIndex_(T&& t, F& f, TIndexSeq<Is...>) {(f(t, Is), ...);}
template<typename T, typename F, size_t... Is> constexpr auto TransformEachField_(T&& t, F& f, TIndexSeq<Is...>)
{return Tuple<decltype(f(get<Is>(t)))...>{f(get<Is>(t))...};}
}

template<typename T, typename F, typename = Requires<CStaticLengthContainer<T>>>
constexpr void ForEachField(T&& t, F&& f)
{z_D::ForEachField_(Forward<T>(t), f, TMakeIndexSeq<StaticLength<T>>());}

template<typename T, typename F, typename = Requires<CStaticLengthContainer<T>>>
constexpr void ForEachFieldIndex(const T& t, F&& f)
{z_D::ForEachFieldIndex_(t, f, TMakeIndexSeq<StaticLength<T>>());}

template<typename T, typename F, typename = Requires<CStaticLengthContainer<T>>>
constexpr auto TransformEachField(const T& t, F&& f)
{return z_D::TransformEachField_(t, f, TMakeIndexSeq<StaticLength<T>>());}

namespace z_D {
template<typename Tuple1, typename Tuple2, size_t... I>
constexpr bool TupleEqual(const Tuple1& lhs, const Tuple2& rhs, TIndexSeq<I...>)
{
	bool result = true;
	(result = result && get<I>(lhs) == get<I>(rhs))...;
	return result;
}
template<typename Tuple1, typename Tuple2, size_t... I>
constexpr int TupleCompare(const Tuple1& lhs, const Tuple2& rhs, TIndexSeq<I...>)
{
	int8 result = 0;
	(result = result == -1? -1: result == 1? 1: 
		get<I>(lhs) < get<I>(rhs)? -1:
		get<I>(rhs) < get<I>(lhs)? 1:
		0)...;
	return result;
}
}

template<typename... Ts1, typename... Ts2>
[[nodiscard]] constexpr bool operator==(const Tuple<Ts1...>& lhs, const Tuple<Ts2...>& rhs)
{return z_D::TupleEqual(lhs, rhs, TSeqFor<Ts1...>{});}

template<typename... Ts1, typename... Ts2>
[[nodiscard]] constexpr bool operator!=(const Tuple<Ts1...>& lhs, const Tuple<Ts2...>& rhs) {return !(lhs == rhs);}

template<typename... Ts1, typename... Ts2>
[[nodiscard]] constexpr bool operator<(const Tuple<Ts1...>& lhs, const Tuple<Ts2...>& rhs)
{return z_D::TupleCompare(lhs, rhs, TSeqFor<Ts1...>{}) < 0;}

template<typename... Ts1, typename... Ts2>
[[nodiscard]] constexpr bool operator>(const Tuple<Ts1...>& lhs, const Tuple<Ts2...>& rhs) {return rhs < lhs;}

template<typename... Ts1, typename... Ts2>
[[nodiscard]] constexpr bool operator<=(const Tuple<Ts1...>& lhs, const Tuple<Ts2...>& rhs) {return !(rhs < lhs);}

template<typename... Ts1, typename... Ts2>
[[nodiscard]] constexpr bool operator>=(const Tuple<Ts1...>& lhs, const Tuple<Ts2...>& rhs) {return !(lhs < rhs);}

template<typename... Ts> constexpr auto Tie(Ts& ... args) {return Tuple<Ts&...>(args...);}

namespace z_D {
template <class F, class Tuple, size_t... I>
constexpr decltype(auto) apply_impl(F&& f, Tuple&& t, TIndexSeq<I...>)
{
	return Forward<F>(f)(get<I>(Forward<Tuple>(t))...);
}
}

template<typename F, typename Tuple> constexpr decltype(auto) Apply(F&& f, Tuple&& t)
{
	return z_D::apply_impl(
		Forward<F>(f), Forward<Tuple>(t),
		TMakeIndexSeq<StaticLength<Tuple>>{});
}

template<size_t N, typename T> struct TupleElementEqualsFunctor
{
	const T& Value;
	template<typename TUPLE> [[nodiscard]] constexpr bool operator()(const TUPLE& rhs) const {return Value == get<N>(rhs);}
};

template<size_t N, typename T> [[nodiscard]] constexpr TupleElementEqualsFunctor<N, T> TupleElementEquals(const T& value)
{return TupleElementEqualsFunctor<N, T>{value};}

//TODO: create an equivalent to https://en.cppreference.com/w/cpp/utility/make_from_tuple

/** Call \p map function for the tuple field at \p index.
	@param map A functor callable with any field of \p t. All return value types must have a common type.
	@param t A tuple-like object
	@param index
	@return map(get<index>(t))
*/

INTRA_END
