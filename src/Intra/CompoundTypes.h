#pragma once

#include "Intra/Functional.h"
#include "Intra/Concepts.h"
#include "Intra/Preprocessor.h"
#include "Intra/Meta.h"

namespace Intra { INTRA_BEGIN
INTRA_IGNORE_WARN_COPY_MOVE_CONSTRUCT_IMPLICITLY_DELETED

template<size_t I> constexpr auto At = []<class T>(T&& v) -> decltype(auto)
requires CHasIndex<T> || z_D::CTupleElementDefined<T> || CHasReflectFieldPointersOf<T>
{
	if constexpr(CStaticLengthContainer<T>) static_assert(I < StaticLength<T>);
	if constexpr(CHasIndex<T>) return v[I];
	else if constexpr(z_D::CTupleElementDefined<T>) return get<I>(INTRA_FWD(v));
	else return v.*(operator()(ReflectFieldPointersOf<T>));
};


namespace z_D {
template<size_t I, typename T> struct TupleLeaf
{
	[[no_unique_address]] T Value;

	TupleLeaf() = default;
	//static_assert(!CReference<ValueType>, "Attempt to default construct a reference element in a tuple");

	TupleLeaf(const TupleLeaf& t) = default;
	//static_assert(!CRValueReference<ValueType>, "Can not copy a tuple with rvalue reference member");

	template<typename T1> requires CConstructible<T, T1>
	constexpr explicit TupleLeaf(T1&& t): Value(INTRA_FWD(t))
	{
		static_assert(!CReference<T> ||
			CLValueReference<T> && CLValueReference<T> ||
			CRValueReference<T> && !CLValueReference<T>,
			"Cannot construct a reference element in a tuple with an rvalue.");
	}

	template<typename T> constexpr explicit TupleLeaf(const TupleLeaf<I, T>& t): Value(t.Value) {}
};

template<typename IndexSeq, typename... Ts> struct TupleImpl;
template<size_t... Is, typename... Ts> struct INTRA_EMPTY_BASES TupleImpl<TIndexSeq<Is...>, Ts...>: TupleLeaf<Is, Ts>...
{
	template<size_t... FirstIndices, typename... FirstTypes,
		size_t... LastIndices, typename... LastTypes,
		typename... ValueTypes>
	constexpr explicit TupleImpl(TIndexSeq<FirstIndices...>, TList<FirstTypes...>,
		TIndexSeq<LastIndices...>, TList<LastTypes...>,
		ValueTypes&&... values):
		TupleLeaf<FirstIndices, FirstTypes>(INTRA_FWD(values))...,
		TupleLeaf<LastIndices, LastTypes>()... {}

	template<typename OtherTuple> constexpr TupleImpl(OtherTuple&& t):
		TupleLeaf<Is, Ts>(INTRA_FWD(get<Is>(t)))... {}

	template<typename OtherTuple> constexpr TupleImpl& operator=(OtherTuple&& t)
	{
		((static_cast<TupleLeaf<Is, Ts>*>(this)->Value = INTRA_FWD(get<Is>(t))), ...);
		return *this;
	}

	constexpr TupleImpl& operator=(const TupleImpl& t)
	{
		((static_cast<TupleLeaf<Is, Ts>*>(this)->Value = static_cast<const TupleLeaf<Is, Ts>&>(t).Value), ...);
		return *this;
	}

	template<size_t I> [[nodiscard]] friend constexpr decltype(auto) get(TupleImpl& t) {return get_<I>(t);}
	template<size_t I> [[nodiscard]] friend constexpr decltype(auto) get(const TupleImpl& t) {return get_<I>(t);}
	template<size_t I> [[nodiscard]] friend constexpr decltype(auto) get(TupleImpl&& t) {return INTRA_MOVE(get_<I>(INTRA_MOVE(t)));}

	template<size_t I, typename TupleImplRef> static constexpr auto&& get_(TupleImplRef&& t)
	{
		static_assert(I < sizeof...(Ts), "Field index is too large for this tuple type!");
		return static_cast<TPropagateQualRef<TupleImplRef, z_D::TupleLeaf<I, TPackAt<I, Ts...>>>>(t).Value;
	}
};
}


template<typename... Ts> struct Tuple
{
private:
	template<index_t I> using Leaf = z_D::TupleLeaf<I, TPackAt<I, Ts...>>; //used by MSVC Native Visualizer (natvis)
	using Impl = z_D::TupleImpl<TMakeIndexSeq<sizeof...(Ts)>, Ts...>;
	[[no_unique_address]] Impl impl_;
public:

    //explicit((!CConvertibleTo<const Ts&, Ts> || ...)) //TODO: strange compiler error MSVC
		constexpr Tuple(const Ts&... t): impl_(
		TMakeIndexSeq<sizeof...(Ts)>(),
		TQualRefFieldTListSlice<Tuple, sizeof...(Ts)>(),
		TMakeIndexSeq<0>(),
		TQualRefFieldTListSlice<Tuple, 0>(),
		t...) {}

	template<typename... Us> requires(sizeof...(Us) <= sizeof...(Ts)) &&
		CTListConvertible<TList<Us...>, TListSlice<TList<Ts...>, 0, Min(sizeof...(Us), sizeof...(Ts))>>
	constexpr Tuple(Us&&... u): impl_(TMakeIndexSeq<sizeof...(Us)>(),
		TListSlice<TList<Ts...>, 0, sizeof...(Us)>(),
		TIndexRange<sizeof...(Ts), sizeof...(Us)>(),
		TListSlice<TList<Ts...>, sizeof...(Us), sizeof...(Ts)>(),
		INTRA_FWD(u)...) {}

	template<class OtherTuple> requires CTListConvertible<TQualRefFieldTList<OtherTuple>, TQualRefFieldTList<Tuple>>
	constexpr Tuple(OtherTuple&& t): impl_(INTRA_FWD(t)) {}

	template<class OtherTuple> requires CTListAssignable<TQualRefFieldTList<Tuple>, TQualRefFieldTList<OtherTuple>>
	constexpr Tuple& operator=(OtherTuple&& t)
	{
		impl_.operator=(INTRA_FWD(t));
		return *this;
	}

	// for compatibility with STL and structured bindings
	template<size_t I> [[nodiscard]] friend constexpr decltype(auto) get(Tuple& t) {return get<I>(t.impl_);}
	template<size_t I> [[nodiscard]] friend constexpr decltype(auto) get(const Tuple& t) {return get<I>(t.impl_);}
	template<size_t I> [[nodiscard]] friend constexpr decltype(auto) get(Tuple&& t) {return INTRA_MOVE(get<I>(INTRA_MOVE(t.impl_)));}
};
template<> struct Tuple<>
{
	Tuple() = default;
	Tuple(Tuple&&) = default;
	Tuple(const Tuple&) = default;
	Tuple& operator=(const Tuple&) = default;
	Tuple& operator=(Tuple&&) = default;

	template<class OtherTuple> requires(StaticLength<OtherTuple> == 0)
	constexpr Tuple(OtherTuple&&) {}

	template<class OtherTuple> requires(StaticLength<OtherTuple> == 0)
	constexpr Tuple& operator=(OtherTuple&&) {return *this;}
};
template<typename... Ts> Tuple(Ts...) -> Tuple<Ts...>;

template<typename... Ts1, typename... Ts2>
[[nodiscard]] constexpr bool operator==(const Tuple<Ts1...>& lhs, const Tuple<Ts2...>& rhs)
{
	return [&]<size_t... I>(TIndexSeq<I...>)
	{
		bool result = true;
		((result = result && get<I>(lhs) == get<I>(rhs)), ...);
		return result;
	}(TSeqFor<Ts1...>());
}

template<typename... Ts1, typename... Ts2>
[[nodiscard]] constexpr bool operator!=(const Tuple<Ts1...>& lhs, const Tuple<Ts2...>& rhs) {return !(lhs == rhs);}

template<typename... Ts1, typename... Ts2>
[[nodiscard]] constexpr bool operator<(const Tuple<Ts1...>& lhs, const Tuple<Ts2...>& rhs)
{
	return []<size_t... I>(TIndexSeq<I...>)
	{
		int8 result = 0;
		((result = result == -1? -1: result == 1? 1: 
			get<I>(lhs) < get<I>(rhs)? -1:
			get<I>(rhs) < get<I>(lhs)? 1:
			0), ...);
		return result;
	}(TSeqFor<Ts1...>()) < 0;
}

template<typename... Ts1, typename... Ts2>
[[nodiscard]] constexpr bool operator>(const Tuple<Ts1...>& lhs, const Tuple<Ts2...>& rhs) {return rhs < lhs;}

template<typename... Ts1, typename... Ts2>
[[nodiscard]] constexpr bool operator<=(const Tuple<Ts1...>& lhs, const Tuple<Ts2...>& rhs) {return !(rhs < lhs);}

template<typename... Ts1, typename... Ts2>
[[nodiscard]] constexpr bool operator>=(const Tuple<Ts1...>& lhs, const Tuple<Ts2...>& rhs) {return !(lhs < rhs);}

template<typename... Ts> constexpr auto Tie(Ts&... args) {return Tuple<Ts&...>(args...);}

namespace z_D::ADL {void get(...) {}}

namespace z_D {
template<bool ReturnArray, typename F, typename T, size_t... Is>
constexpr decltype(auto) ForEachFieldGenericHelper(F&& f, T&& obj, TIndexSeq<Is...>) {
	if constexpr((CCallable<decltype(f), decltype(At<Is>(obj)), TIndex<Is>> && ...))
	{
		constexpr bool isAnyReturnTypeVoid = (CVoid<decltype(f(At<Is>(obj), TIndex<Is>()))> || ...);
		if constexpr(isAnyReturnTypeVoid) ((f(At<Is>(obj), TIndex<Is>())), ...);
		else if constexpr(ReturnArray)
		{
			static_assert(CSame<decltype(f(At<Is>(obj), TIndex<Is>()))...>);
			return Array{f(At<Is>(obj), TIndex<Is>())...};
		}
		else return Tuple{f(At<Is>(obj), TIndex<Is>())...};
	}
	else
	{
		constexpr bool isAnyReturnTypeVoid = (CVoid<decltype(f(At<Is>(obj)))> || ...);
		if constexpr(isAnyReturnTypeVoid) (f(At<Is>(obj)), ...);
		else if constexpr(ReturnArray)
		{
			static_assert(CSame<decltype(f(At<Is>(obj)))...>);
			return Array{f(At<Is>(obj))...};
		}
		else return Tuple{f(At<Is>(obj))...};
	}
}

template<bool ReturnArray> constexpr auto ForEachFieldGeneric = []<typename F>(F&& f) {
	return [f = ForwardAsFunc<F>(f)]<CStaticLengthContainer Object>(Object&& obj) mutable {
		return ForEachFieldGenericHelper<ReturnArray>(f, obj, TMakeIndexSeq<StaticLength<Object>>());
	};
};
}
constexpr auto ForEachField = z_D::ForEachFieldGeneric<false>;
constexpr auto ForEachFieldToArray = z_D::ForEachFieldGeneric<true>;

constexpr auto ApplyPackedArgs = []<typename F>(F&& func) {
	return [func = ForwardAsFunc<F>(func)]<CStaticLengthContainer C>(C&& slc) mutable {
		return [&]<size_t... Is>(TIndexSeq<I...>) {
			return func(At<I>(slc)...);
		}();
	};
};

template<typename T> constexpr auto ConstructWithPackedArgs = []<CStaticLengthContainer S>(S&& slc) {
	return [&]<size_t... Is>(TIndexSeq<Is...>)
	{
		return T(At<Is>(INTRA_FWD(slc))...);
	}(TMakeIndexSeq<StaticLength<S>>());
};

namespace z_D {
template<typename F, typename S, size_t... Is> constexpr decltype(auto) ForFieldAtRuntimeHelper(Index index, [[maybe_unused]] F&& f, S&& t, TIndexSeq<Is...>)
{
	using CommonReturnType = TMappedFieldCommonRef<S, F>; //void if no common type exists
	constexpr bool callableForFieldTypesWithoutIndex = (CCallable<F, decltype(At<Is>(INTRA_FWD(t)))> && ...);
	constexpr bool callableForFieldTypesWithIndex = (CCallable<F, decltype(At<Is>(INTRA_FWD(t))), TIndex<Is>> && ...);
	static_assert(callableForFieldTypesWithoutIndex || callableForFieldTypesWithIndex);
	if constexpr(sizeof...(Is) <= 30)
	{
		//Efficient macro based solution which takes advantage of RVO and a jump table generated by most compilers
		switch(size_t(index))
		{
	#define INTRA_CASE_LINE(i) case i: if constexpr(i < sizeof...(Is)) \
		if constexpr(callableForFieldTypesWithIndex) \
			return static_cast<CommonReturnType>(f(At<i>(INTRA_FWD(t)), TIndex<i>())); \
		else return static_cast<CommonReturnType>(f(At<i>(INTRA_FWD(t))));
		INTRA_MACRO_REPEAT(30, INTRA_CASE_LINE,);
	#undef INTRA_CASE_LINE
		}
		UnreachableCode(Unsafe);
	}
	else if constexpr(CVoid<CommonReturnType>)
	{
		if constexpr(callableForFieldTypesWithIndex) ((index == Is &&
			(void(
				f(At<Is>(INTRA_FWD(t)), TIndex<Is>())
				), 1)) || ...);
		else ((index == Is &&
			(void(
				f(At<Is>(INTRA_FWD(t)))
				), 1)) || ...);
	}
	else
	{
		//Less efficient solution (NRVO + move assignment instead of RVO), supports unlimited number of fields but has additional common type requirements
		static_assert(CMoveConstructible<CommonReturnType>);
		static_assert(CConstructible<CommonReturnType> || CMoveAssignable<CommonReturnType>);
		TSelect<CommonReturnType,
			Optional<CommonReturnType>,
			CConstructible<CommonReturnType>> ret;
		if constexpr(callableForFieldTypesWithIndex) ((index == Is &&
			(void(
				ret = f(At<Is>(INTRA_FWD(t)), TIndex<Is>())
				), 1)) || ...);
		else ((index == Is &&
			(void(
				ret = f(At<Is>(INTRA_FWD(t)))
				), 1)) || ...);
		if constexpr(CConstructible<CommonReturnType>) return ret;
		else return ret.Unwrap();
	}
}
}
constexpr auto ForFieldAtRuntime = []<typename F1>(Index index, F1&& f) {
	return [index, f = ForwardAsFunc<F1>(f)]<CStaticLengthContainer S>(S&& t) mutable -> decltype(auto) {
		INTRA_PRECONDITION(size_t(index) < StaticLength<S>);
		return z_D::ForFieldAtRuntimeHelper(index, f, INTRA_FWD(t), TMakeIndexSeq<StaticLength<S>>());
	};
};


INTRA_IGNORE_WARN_COPY_MOVE_IMPLICITLY_DELETED
INTRA_IGNORE_WARNS_MSVC(4582); //constructor is not implicitly called

/// Contiguous container stores either 0 or 1 elements without dynamic allocation.
template<typename T> class Optional
{
public:
	constexpr Optional(TUndefined = Undefined) noexcept: mVal(Undefined) {};
	
	template<typename... Args> requires CConstructible<T, Args&&...>
	explicit constexpr Optional(decltype(Construct), Args&&... args):
		mVal(Construct, INTRA_FWD(args)...), mHasValue(true) {}

	template<typename... Args> requires CConstructible<T, Args&&...>
	explicit constexpr Optional(Args&&... args): Optional(Construct, INTRA_FWD(args)...) {}

	template<typename U> explicit(!CConvertibleTo<U&&, T>) constexpr Optional(U&& value): Optional(INTRA_FWD(value)) {}

	Optional(Optional&& rhs) requires CTriviallyDestructible<T> = default;
	Optional(const Optional& rhs) requires CTriviallyDestructible<T> = default;

	template<CSameUnqualRef<Optional> Me> requires(!CTriviallyDestructible<T>) Optional(Me&& rhs)
	{
		if(!rhs.mHasValue) return;
		new(Construct, AddressOf(mVal.value)) T(static_cast<TPropagateQualRef<Me&&, T>>(rhs.mVal.value));
		mHasValue = true;
	}

	INTRA_CONSTEXPR_DESTRUCTOR ~Optional() {if(mHasValue) mVal.value.~T();}
	~Optional() requires CTriviallyDestructible<T> = default;

	constexpr T& Set(const T& rhs) requires CCopyAssignable<T>
	{
		if(mHasValue) mVal.value = rhs;
		else new(Construct, AddressOf(mVal.value)) T(rhs);
		return mVal.value;
	}

	constexpr T& Set(T&& rhs) requires CMoveAssignable<T>
	{
		if(mHasValue) mVal.value = Move(rhs);
		else new(Construct, AddressOf(mVal.value)) T(Move(rhs));
		return BASE::mVal.value;
	}

	/// Construct the contained value in-place. Destruct an existing contained value if any.
	template<typename... Args> requires CConstructible<T, Args&&...>
	constexpr T& Emplace(Args&&... args)
	{
		if(BASE::mHasValue) BASE::mVal.value.~T();
		new(Construct, AddressOf(BASE::mVal.value)) T(INTRA_FWD(args)...);
		return BASE::mVal.value;
	}

	constexpr Optional& operator=(const Optional& rhs) requires CCopyAssignable<T>
	{
		if(rhs.mHasValue) return operator=(rhs.mVal.value);
		if(mHasValue) BASE::mVal.value.~T();
		mHasValue = false;
		return *this;
	}

	constexpr Optional& operator=(Optional&& rhs) requires CMoveAssignable<T>
	{
		if(rhs.mHasValue) return operator=(INTRA_MOVE(rhs.mVal.value));
		if(mHasValue) mVal.value.~T();
		mHasValue = false;
		return *this;
	}

	[[nodiscard]] constexpr bool operator==(TUndefined) const {return !mHasValue;}
	[[nodiscard]] constexpr bool operator!=(TUndefined) const {return !operator==(Undefined);}
	[[nodiscard]] constexpr explicit operator bool() const {return !operator==(Undefined);}

	[[nodiscard]] constexpr T* Data() {return AddressOf(mVal.value);}
	[[nodiscard]] constexpr const T* Data() const {return AddressOf(mVal.value);}
	[[nodiscard]] constexpr index_t Length() const {return index_t(mHasValue);}

	/// @return true if both objects have equal values or both have no value.
	[[nodiscard]] constexpr bool operator==(const Optional& rhs) const requires CEqualityComparable<T>
	{
		return mHasValue == rhs.mHasValue &&
			(!mHasValue || mVal.value == rhs.mVal.value);
	}

	///@{
	/// @return contained object. Must be called only on non-empty Optional!
	[[nodiscard]] constexpr T& Unwrap()
	{
		INTRA_PRECONDITION(*this != Undefined);
		return mVal.value;
	}

	[[nodiscard]] constexpr const T& Unwrap() const
	{
		INTRA_PRECONDITION(*this != Undefined);
		return mVal.value;
	}
	///@}

private:
	union storage
	{
		char dummy[1];
		T value;
		constexpr storage(TUndefined) noexcept: dummy{} {};
		template<typename... Args> constexpr storage(decltype(Construct), Args&&... args): value(INTRA_FWD(args)...) {}
		storage(const storage&) = default;
		storage(storage&&) = default;
		INTRA_CONSTEXPR_DESTRUCTOR ~storage() {}
		~storage() requires CTriviallyDestructible<T> = default;
	} mVal;
	bool mHasValue = false;
};

template<typename T> class Optional<T&>
{
	T* mVal = nullptr;
public:
	Optional() = default;
	constexpr Optional(TUndefined) noexcept {}
	explicit constexpr Optional(T& v) noexcept: mVal(&v) {}

	[[nodiscard]] constexpr bool operator==(TUndefined) const noexcept {return mVal == nullptr;}
	[[nodiscard]] constexpr bool operator!=(TUndefined) const noexcept {return mVal != nullptr;}
	[[nodiscard]] constexpr explicit operator bool() const noexcept {return mVal != nullptr;}

	[[nodiscard]] constexpr T* Data() noexcept {return mVal;}
	[[nodiscard]] constexpr const T* Data() const noexcept {return mVal;}
	[[nodiscard]] constexpr index_t Length() const noexcept {return index_t(mVal != nullptr);}

	bool operator==(const Optional& rhs) const = default;

	INTRA_FORCEINLINE constexpr T& Set(const T& rhs) noexcept {mVal = &rhs;}
	INTRA_FORCEINLINE constexpr T& Emplace(const T& rhs) noexcept {Set(rhs);}

	///@{
	/// @return contained object. Must be called only on non-empty Optional!
	[[nodiscard]] constexpr T& Unwrap()
	{
		INTRA_PRECONDITION(*this != Undefined);
		return *mVal;
	}

	[[nodiscard]] constexpr const T& Unwrap() const
	{
		INTRA_PRECONDITION(*this != Undefined);
		return *mVal;
	}
	///@}
};
template<typename T> Optional(T) -> Optional<T>;

template<typename T> constexpr Optional<T&> OptRef(T& rhs) {return Optional<T&>(rhs);}

#if INTRA_CONSTEXPR_TEST
static_assert(Optional(5) == Optional<int>(5));
static_assert(Optional<int>(5) != Optional<int>{4});
static_assert(Optional<int>() == Undefined);
static_assert(Optional<int>(0) != Optional<int>());
static_assert(Optional<int>(Construct).Unwrap() == 0);
static_assert(Optional<int>(7).Unwrap() == 7);
static_assert(Optional(7.6f).Unwrap() == 7.6f);
//static_assert(Optional<int>().Unwrap() != 0, "Getting the value of the empty optional is an error!");
#endif


namespace z_D {
template<typename TFirst, typename... TRest> struct VariantStorage
{
	using Types = TList<TFirst, TRest...>;
	union
	{
		TRemoveConst<TFirst> First;
		VariantStorage<TRest...> Rest;
	};

	constexpr VariantStorage() noexcept {}

	INTRA_CONSTEXPR_DESTRUCTOR ~VariantStorage() {}
	~VariantStorage() requires(CTriviallyDestructible<TFirst> && CTriviallyDestructible<TRest> && ...) = default;

	template<typename... Args>
	constexpr explicit VariantStorage(TIndex<0>, Args&&... args) noexcept(
		CNothrowConstructible<TFirst, TRest...>):
		First(INTRA_FWD(args)...) {}

	template<size_t I, typename... Args> requires(I > 0)
	constexpr explicit VariantStorage(TIndex<I>, Args&&... args) noexcept(
		CNothrowConstructible<VariantStorage<TRest...>, TIndex<I - 1>, Args...>):
		Rest(TIndex<I - 1>{}, INTRA_FWD(args)...) {}

	VariantStorage(VariantStorage&&) = default;
	VariantStorage(const VariantStorage&) = default;
	VariantStorage& operator=(VariantStorage&&) = default;
	VariantStorage& operator=(const VariantStorage&) = default;
};

template<size_t I, typename T> constexpr decltype(auto) getStorage_(T&& vs) noexcept
{
	using ReturnType = TPropagateQualRef<T, TListAt<I, typename TRemoveConstRef<T>::Types>>;
	if constexpr(I == 0) return static_cast<ReturnType>(vs.First);
	else if constexpr(I == 1) return static_cast<ReturnType>(vs.Rest.First);
	else if constexpr(I == 2) return static_cast<ReturnType>(vs.Rest.Rest.First);
	else if constexpr(I == 3) return static_cast<ReturnType>(vs.Rest.Rest.Rest.First);
	else if constexpr(I == 4) return static_cast<ReturnType>(vs.Rest.Rest.Rest.Rest.First);
	else if constexpr(I == 5) return static_cast<ReturnType>(vs.Rest.Rest.Rest.Rest.Rest.First);
	else if constexpr(I == 6) return static_cast<ReturnType>(vs.Rest.Rest.Rest.Rest.Rest.Rest.First);
	else if constexpr(I == 7) return static_cast<ReturnType>(vs.Rest.Rest.Rest.Rest.Rest.Rest.Rest.First);
	else return getStorage_<I-8>(vs.Rest.Rest.Rest.Rest.Rest.Rest.Rest.Rest);
}
template<size_t I, typename T> requires CInstanceOfTemplate<TRemoveConstRef<T>, VariantStorage>
constexpr decltype(auto) get(T&& vs) noexcept {return getStorage_<I>(INTRA_FWD(vs));}

template<size_t I, typename T> struct TVariantInitOverload_
{
	using F = TList<TIndex<I>, T>(*)(T);
	operator F();
};

template<class IndexSeq, typename... Ts> struct TVariantInitOverloads_;

template<size_t... Is, typename... Ts>
struct TVariantInitOverloads_<TIndexSeq<Is...>, Ts...>: TVariantInitOverload_<Is, Ts>... {};

INTRA_DEFINE_SAFE_DECLTYPE_T_ARGS(VariantInitHelper, TVariantInitOverloads_<TSeqFor<Args...>, Args...>{}(Val<T>()));
}

template<typename... Ts> class Variant: private z_D::VariantStorage<Ts...>
{
	using Storage = z_D::VariantStorage<Ts...>;
	template<typename T> using InitType = TListAt<1, z_D::VariantInitHelper<T, Ts...>>;
	template<typename T> static constexpr size_t InitIndex = TListAt<0, z_D::VariantInitHelper<T, Ts...>>::_;
	static_assert(((CObject<Ts> && !CArray<Ts> && CDestructible<Ts>) && ...));
	static_assert(sizeof...(Ts) != 0);
public:
	constexpr Variant() noexcept(CNothrowConstructible<TPackFirst<Ts...>>)
		requires CConstructible<TPackFirst<Ts...>>: Variant(ConstructAt<0>) {}

	Variant(const Variant& rhs) requires CTriviallyCopyConstructible<Storage> = default;
	constexpr Variant(const Variant& rhs): Storage(rhs) {
		//TODO: implement
	}
	Variant(Variant&& rhs) requires CTriviallyMoveConstructible<Storage> = default;
	constexpr Variant(Variant&& rhs) noexcept((CNothrowMoveConstructible<Ts> && ...)):
		Storage(INTRA_MOVE(rhs)) {
		//TODO: implement
	}

	template<typename T> requires (sizeof...(Ts) != 0) &&
		(!CSame<TUnqualRef<T>, Variant>) &&
		(!CInstanceOfTemplate<TUnqualRef<T>, TConstructT>) &&
		(!CConstructAt<TUnqualRef<T>>) &&
		CConstructible<InitType<T>, T>
	constexpr Variant(T&& t) noexcept(CNothrowConstructible<InitType<T>, T>):
		Variant(TConstructAt<InitIndex<T>>, INTRA_FWD(t)) {}

	template<typename T, typename... Args, size_t I = TListFindUnique<Variant, T>>
	requires (I != sizeof...(Ts)) && CConstructible<T, Args...>
	constexpr explicit Variant(TConstructT<T>, Args&&... args)
		noexcept(CNothrowConstructible<T, Args...>):
		Variant(ConstructAt<I>, INTRA_FWD(args)...) {}

	template<size_t I, typename... Args> requires CConstructible<TPackAt<I, Ts...>, Args...>
	constexpr explicit Variant(TConstructAt<I>, Args&&... args)
		noexcept(CNothrowConstructible<TPackAt<I, Ts...>, Args...>):
		VariantStorage<Ts...>(TIndex<I>{}, INTRA_FWD(args)...),
		mAlternativeIndex(I) {}

	~Variant() requires (CTriviallyDestructible<Ts> && ...) = default;
	INTRA_CONSTEXPR_DESTRUCTOR ~Variant() {destroy();}

	template<typename T> requires
		(!CSame<TUnqualRef<T>, Variant>) &&
		CConstructible<InitType<T>, T> &&
		CAssignable<InitType<T>&, T>
	Variant& operator=(T&& rhs) noexcept(CNothrowAssignable<InitType<T>&, T> && CNothrowConstructible<InitType<T>, T>)
	{
		constexpr size_t I = InitIndex<T>;
		if(mAlternativeIndex == I)
		{
			auto& storage = z_D::getStorage_<I>(static_cast<Storage&>(*this));
			storage = INTRA_FWD(rhs);
			return *this;
		}
		using DstT = z_D::VariantInitType<T, Ts...>;
		if constexpr(!CNothrowConstructible<DstT, T> && CNothrowMoveConstructible<DstT>)
			Emplace<DstT>(DstT(INTRA_FWD(rhs)));
		else Emplace<DstT>(INTRA_FWD(rhs));
		return *this;
	}

	template<typename T, typename... Args, size_t I = TListFindUniqueIndex<Variant, T>>
		requires (I != sizeof...(Ts)) && CConstructible<T, Args...>
	T& Emplace(Args&&... args) noexcept(CNothrowConstructible<T, Args...>) {return Emplace<I>(INTRA_FWD(args)...);}

	template<size_t I, class... Args> requires CConstructible<TPackAt<I, Ts...>, Args...>
	TPackAt<I, Ts...>& Emplace(Args&&... args) noexcept(CNothrowConstructible<TPackAt<I, Ts...>, Args...>)
	{
		reset();
		auto& storage = z_D::getStorage_<I>(static_cast<Storage&>(*this));
		new(Construct, AddressOf(storage)) TPackAt<I, Ts...>(INTRA_FWD(args)...);
		mAlternativeIndex = I;
		return storage;
	}

	[[nodiscard]] constexpr bool Empty() const noexcept {return mAlternativeIndex == sizeof...(Ts);}
	[[nodiscard]] constexpr index_t CurrentAlternativeIndex() const noexcept {return mAlternativeIndex;}

private:
	using IndexT = TSelect<uint8, int32, (sizeof...(Ts) < 256)>;
	IndexT mAlternativeIndex = sizeof...(Ts);

	template<size_t I> void destroy() noexcept
	{
		INTRA_PRECONDITION(I == CurrentAlternativeIndex());
		if constexpr(I != sizeof...(Ts) && !CTriviallyDestructible<TPackAt<I, Ts...>>)
		{
			using T_I = TPackAt<I, Ts...>;
			getStorage_<I>(*this).~T_I();
		}
	}

	void destroy() noexcept
	{
		if constexpr(!(CTriviallyDestructible<Ts> && ...))
			if(!Empty()) ForFieldAtRuntime(mAlternativeIndex, []<typename T>(T& ref) noexcept {ref.~T();})(*this);
	}

	void reset() noexcept
	{
		destroy();
		mAlternativeIndex = sizeof...(Ts);
	}

	template<size_t I> void reset() noexcept
	{
		if constexpr(I != sizeof...(Ts))
		{
			destroy<I>();
			mAlternativeIndex = sizeof...(Ts);
		}
	}

	template<class Var> requires CSame<TUnqualRef<Var>, Variant>
	void constructFrom(Var&& rhs) noexcept((CNothrowConstructible<Ts, TPropagateQualRef<Var, Ts>> && ...))
	{
		if(rhs.Empty()) return;
		ForFieldAtRuntime(rhs.mAlternativeIndex, [&, self=this]<size_t I, typename T>(T&& val, TIndex<I>) {
			using DstT = TUnqualRef<T>;
			DstT* storage = AddressOf(z_D::getStorage_<I>(*self));
			new(Construct, storage) DstT(INTRA_FWD(val));
		})(static_cast<TPropagateQualRef<decltype(rhs), VariantStorage<Ts...>>&>(rhs));
		mAlternativeIndex = rhs.mAlternativeIndex;
	}

	template<CSameUnqualRef<Variant> Var> void assignFrom(Var&& rhs) noexcept((
		(CNothrowConstructible<Ts, TPropagateQualRef<Var, Ts>> &&
			CNothrowAssignable<Ts, TPropagateQualRef<Var, Ts>>) && ...))
	{
		if(rhs.Empty())
		{
			reset();
			return;
		}
		ForFieldAtRuntime(rhs.mAlternativeIndex, [&, self=this]<size_t I, typename T>(T&& val, TIndex<I>) {
			using DstT = TUnqualRef<T>;
			DstT& storage = z_D::getStorage_<I>(*self);
			if(mAlternativeIndex == I)
			{
				storage = INTRA_FWD(val);
				return;
			}
			if constexpr(CRValueReference<T>)
			{
				reset();
				new(Construct, AddressOf(storage)) DstT(INTRA_MOVE(val));
			}
			else if constexpr(!CNothrowConstructible<DstT, T> && CNothrowMoveConstructible<DstT>)
			{
				DstT tempCopy = val;
				reset();
				new(Construct, AddressOf(storage)) DstT(INTRA_MOVE(tempCopy));
			}
			else
			{
				reset();
				new(Construct, AddressOf(storage)) DstT(val);
			}
			mAlternativeIndex = I;
		})(static_cast<TPropagateQualRef<decltype(rhs), Storage>&>(rhs));
	}
};

constexpr auto VariantVisit = []<typename F>(F&& f) {
	return [f = ForwardAsFunc<F>()](auto&& arg0, auto&&... args) {
		//TODO
		//if constexpr(CVariant<decltype(arg0)>)
		if constexpr(sizeof...(args) == 0) return f(arg0);
	};
};

#ifdef INTRA_CONSTEXPR_TEST //TODO:
//static_assert(Variant<int, float, double>(3.14f) == Variant<int, float, double>(ConstructAt<1>, 3.14f));
#endif

} INTRA_END
