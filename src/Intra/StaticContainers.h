#pragma once

#include "Intra/Functional.h"
#include "Intra/Concepts.h"
#include "Intra/Operations.h"
#include "Intra/Preprocessor.h"

INTRA_BEGIN
INTRA_IGNORE_WARN_COPY_MOVE_CONSTRUCT_IMPLICITLY_DELETED

namespace z_D {
template<size_t I, typename T> struct TupleLeaf
{
protected:
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
		(this->Value = INTRA_FWD(get<Is>(t)), ...);
		return *this;
	}

	constexpr TupleImpl& operator=(const TupleImpl& t)
	{
		(this->Value = static_cast<const TupleLeaf<Is, Ts>&>(t).Value, ...);
		return *this;
	}

	template<index_t I> friend constexpr auto&& get(TupleImpl& t)
	{
		static_assert(I < sizeof...(Ts), "Field index is too large for this tuple type!");
		using Type = TPackAt<I, Ts...>;
		return static_cast<z_D::TupleLeaf<I, Type>&>(t).Value;
	}
	template<size_t I> friend constexpr auto&& get(const TupleImpl& t)
	{
		static_assert(I < sizeof...(Ts), "Field index is too large for this tuple type!");
		using Type = TPackAt<I, Ts...>;
		return static_cast<const z_D::TupleLeaf<I, Type>&>(t).Value;
	}
	template<size_t I> friend constexpr auto&& get(TupleImpl&& t)
	{
		static_assert(I < sizeof...(Ts), "Field index is too large for this tuple type!");
		using Type = TPackAt<I, Ts...>;
		return static_cast<Type&&>(static_cast<z_D::TupleLeaf<I, Type>&&>(t).Value);
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
	
	constexpr Tuple(const Ts&... t) requires(CConvertibleTo<const Ts&, Ts> && ...): impl_(
		TMakeIndexSeq<sizeof...(Ts)>(),
		TQualRefFieldTListSlice<Tuple, sizeof...(Ts)>(),
		TMakeIndexSeq<0>(),
		TQualRefFieldTListSlice<Tuple, 0>(),
		t...) {}

    explicit constexpr Tuple(const Ts&... t) requires(CConvertibleTo<const Ts&, Ts> && ...): impl_(
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

/// Get I-th value of a static length container
template<size_t I> constexpr auto FieldAt = []<CStaticLengthContainer S>(S&& slc) {
	if constexpr(z_D::CTupleElementDefined<S>) return get<I>(INTRA_FWD(slc));
	else return slc.*(operator()(FieldPointersOf<S>));
};

constexpr auto ApplyPackedArgs = []<typename F>(F&& func) {
	return [func = ForwardAsFunc<F>(func)]<CStaticLengthContainer C>(C&& slc) {
		return [&]<size_t... Is>(TIndexSeq<I...>) {
			return func(FieldAt<I>(slc)...);
		}();
	};
};

template<typename T> constexpr auto ConstructWithPackedArgs = []<CStaticLengthContainer S>(S&& slc) {
	return [&](TIndexSeq<I...>)
	{
		return T(FieldAt<I>(INTRA_FWD(slc))...);
	}(TMakeIndexSeq<StaticLength<S>>());
};

constexpr auto ForFieldAtRuntime = []<typename F1>(Index index, F1&& f) {
	return [index, f = ForwardAsFunc<F1>(f)]<CStaticLengthContainer S>(S&& t) {
		INTRA_PRECONDITION(size_t(index) < StaticLength<S>);
		using CommonReturnType = TMappedFieldCommon<S, F>; //void if no common type exists
		using namespace z_D::ADL;
		return [&]<size_t... Is>(TIndexSeq<Is...>) {
			constexpr bool callableForFieldTypesWithoutIndex = (CCallable<F, decltype(FieldAt<Is>(INTRA_FWD(t)))> && ...);
			constexpr bool callableForFieldTypesWithIndex = (CCallable<F, decltype(FieldAt<Is>(INTRA_FWD(t))), TIndex<Is>> && ...);
			static_assert(callableForFieldTypesWithoutIndex || callableForFieldTypesWithIndex);
			if constexpr(sizeof...(Is) <= 30)
			{
				//Efficient macro based solution which takes advantage of RVO and a jump table generated by most compilers
				switch(size_t(index))
				{
			#define INTRA_CASE_LINE(i) case i: if constexpr(i < sizeof...(Is)) \
				if constexpr(callableForFieldTypesWithIndex) \
					return CommonReturnType(Func(FieldAt<i>(INTRA_FWD(t)), TIndex<i>())); \
				else return CommonReturnType(Func(FieldAt<i>(INTRA_FWD(t))));
				INTRA_MACRO_REPEAT(30, INTRA_CASE_LINE,);
			#undef INTRA_CASE_LINE
				default: return CommonReturnType();
				}
			}
			else if constexpr(CVoid<CommonReturnType>)
			{
				if constexpr(callableForFieldTypesWithIndex) ((index == Is &&
					(void(
						Func(FieldAt<Is>(INTRA_FWD(t)), TIndex<Is>())
						), 1)) || ...);
				else ((index == Is &&
					(void(
						Func(FieldAt<Is>(INTRA_FWD(t)))
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
						ret = Func(FieldAt<Is>(INTRA_FWD(t)), TIndex<Is>())
						), 1)) || ...);
				else ((index == Is &&
					(void(
						ret = Func(FieldAt<Is>(INTRA_FWD(t)))
						), 1)) || ...);
				if constexpr(CConstructible<CommonReturnType>) return ret;
				else return ret.Unwrap();
			}
		}(TMakeIndexSeq<StaticLength<S>>());
	};
};


INTRA_IGNORE_WARN_COPY_MOVE_IMPLICITLY_DELETED
INTRA_IGNORE_WARNS_MSVC(4582); //constructor is not implicitly called
namespace z_D {
template<typename T, bool = CTriviallyDestructible<T>> class OptionalBase
{
protected:
	union storage
	{
		char dummy[1];
		T value;
		constexpr storage(TUndefined) noexcept: dummy{} {};
		template<typename... Args> constexpr storage(decltype(Construct), Args&&... args):
			value(INTRA_FWD(args)...) {}
		constexpr storage(const storage&) = default;
		constexpr storage(storage&&) = default;
	} mVal;
	bool mHasValue;

	constexpr OptionalBase(TUndefined) noexcept: mVal(Undefined), mHasValue(false) {};
	template<typename... Args> constexpr OptionalBase(Args&&... args): mVal(Construct, INTRA_FWD(args)...), mHasValue(true) {}
	constexpr OptionalBase(OptionalBase&& rhs) = default;
	constexpr OptionalBase(const OptionalBase& rhs) = default;
};

template<typename T> class OptionalBase<T, false>
{
protected:
	union storage
	{
		char dummy[1];
		T value;
		constexpr storage(TUndefined) noexcept: dummy{} {};
		template<typename... Args> constexpr storage(decltype(Construct), Args&&... args):
			value(INTRA_FWD(args)...) {}
		~storage() {}
	} mVal;
	bool mHasValue = false;

	OptionalBase(TUndefined) noexcept: mVal(Undefined), mHasValue(false) {};
	template<typename... Args> OptionalBase(Args&&... args): mVal(Construct, INTRA_FWD(args)...), mHasValue(true) {}
	~OptionalBase() {if(mHasValue) mVal.value.~T();}

	template<CSameIgnoreCVRef<OptionalBase> Me> OptionalBase(Me&& rhs)
	{
		if(!rhs.mHasValue) return;
		new(Construct, AddressOf(mVal.value)) T(INTRA_FWD(rhs.mVal.value));
		mHasValue = true;
	}
};
}

/// Contiguous container stores either 0 or 1 elements without dynamic allocation.
template<typename T> class Optional: public z_D::OptionalBase<T>
{
	using BASE = z_D::OptionalBase<T>;
public:
	constexpr Optional(TUndefined = Undefined): BASE(Undefined) {}
	
	template<typename... Args> requires CConstructible<T, Args&&...>
	explicit constexpr Optional(decltype(Construct), Args&&... args): BASE(INTRA_FWD(args)...) {}

	template<typename... Args> requires CConstructible<T, Args&&...>
	explicit constexpr Optional(Args&&... args): BASE(INTRA_FWD(args)...) {}

	template<typename U> requires CConvertibleTo<U&&, T> constexpr Optional(U&& value): BASE(INTRA_FWD(value)) {}

	template<typename U> requires(!CConvertibleTo<U&&, T>)
	explicit constexpr Optional(U&& value): BASE(INTRA_FWD(value)) {}

	constexpr Optional(Optional&&) = default;
	constexpr Optional(const Optional&) = default;

	constexpr T& Set(const T& rhs) requires CCopyAssignable<T>
	{
		if(BASE::mHasValue) BASE::mVal.value = rhs;
		else new(Construct, AddressOf(BASE::mVal.value)) T(rhs);
		return BASE::mVal.value;
	}

	constexpr T& Set(T&& rhs) requires CMoveAssignable<T>
	{
		if(BASE::mHasValue) BASE::mVal.value = Move(rhs);
		else new(Construct, AddressOf(BASE::mVal.value)) T(Move(rhs));
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
		if(BASE::mHasValue) BASE::mVal.value.~T();
		BASE::mHasValue = false;
		return *this;
	}

	constexpr Optional& operator=(Optional&& rhs) requires CMoveAssignable<T>
	{
		if(rhs.mHasValue) return operator=(INTRA_MOVE(rhs.mVal.value));
		if(BASE::mHasValue) BASE::mVal.value.~T();
		BASE::mHasValue = false;
		return *this;
	}

	[[nodiscard]] constexpr bool operator==(TUndefined) const {return !BASE::mHasValue;}
	[[nodiscard]] constexpr bool operator!=(TUndefined) const {return !operator==(Undefined);}
	[[nodiscard]] constexpr explicit operator bool() const {return !operator==(Undefined);}

	[[nodiscard]] constexpr T* Data() {return AddressOf(BASE::mVal.value);}
	[[nodiscard]] constexpr const T* Data() const {return AddressOf(BASE::mVal.value);}
	[[nodiscard]] constexpr index_t Length() const {return index_t(BASE::mHasValue);}

	/// @return true if both objects have equal values or both have no value.
	[[nodiscard]] constexpr bool operator==(const Optional& rhs) const requires CEqualityComparable<T>
	{
		return BASE::mHasValue == rhs.mHasValue &&
			(!BASE::mHasValue || BASE::mVal.value == rhs.mVal.value);
	}

	[[nodiscard]] constexpr bool operator!=(const Optional& rhs) const requires CEqualityComparable<T> {return !operator==(rhs);}

	///@{
	/// @return contained object. Must be called only on non-empty Optional!
	[[nodiscard]] constexpr T& Unwrap()
	{
		INTRA_PRECONDITION(*this != Undefined);
		return BASE::mVal.value;
	}

	[[nodiscard]] constexpr const T& Unwrap() const
	{
		INTRA_PRECONDITION(*this != Undefined);
		return BASE::mVal.value;
	}
	///@}

	[[nodiscard]] constexpr T GetOr(T nullValue = T()) const
	{
		if(!*this) return nullValue;
		return BASE::mVal.value;
	}
};

template<typename T> class Optional<T&>
{
	T* mVal = null;
public:
	Optional() = default;
	constexpr Optional(TUndefined) noexcept {}
	explicit constexpr Optional(T& v) noexcept: mVal(&v) {}

	[[nodiscard]] constexpr bool operator==(TUndefined) const noexcept {return mVal == null;}
	[[nodiscard]] constexpr bool operator!=(TUndefined) const noexcept {return mVal != null;}
	[[nodiscard]] constexpr explicit operator bool() const noexcept {return mVal != null;}

	[[nodiscard]] constexpr T* Data() noexcept {return mVal;}
	[[nodiscard]] constexpr const T* Data() const noexcept {return mVal;}
	[[nodiscard]] constexpr index_t Length() const noexcept {return index_t(mVal != null);}

	/// @return true if both objects have equal values or both have no value.
	[[nodiscard]] constexpr bool operator==(const Optional& rhs) const {return mVal == rhs.mVal;}
	[[nodiscard]] constexpr bool operator!=(const Optional& rhs) const {return !operator==(rhs);}

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
template<typename, typename... Ts> class VariantStorage_ {};
template<typename... Ts> using VariantStorage = VariantStorage_<TValue<(CTriviallyDestructible<Ts> && ...)>, Ts...>;

template<typename TFirst, typename... TRest> struct VariantStorage_<TValue<true>, TFirst, TRest...>
{
	using Types = TList<TFirst, TRest...>;
	union
	{
		TRemoveConst<TFirst> First;
		VariantStorage_<TRest...> Rest;
	};

	constexpr VariantStorage_() noexcept {}

	template<typename... Args>
	constexpr INTRA_FORCEINLINE explicit VariantStorage_(TIndex<0>, Args&&... args) noexcept(
		CNothrowConstructible<TFirst, TRest...>):
		First(static_cast<Args&&>(args)...) {}

	template<size_t I, typename... Args> requires(I > 0)
	constexpr INTRA_FORCEINLINE explicit VariantStorage_(TIndex<I>, Args&&... args) noexcept(
		CNothrowConstructible<VariantStorage_<TRest...>, TIndex<I - 1>, Args...>):
		Rest(TIndex<I - 1>{}, static_cast<Args&&>(args)...) {}
};
template<typename TFirst, typename... TRest> struct VariantStorage_<TValue<false>, TFirst, TRest...>
{
	using Types = TList<TFirst, TRest...>;
	union
	{
		TRemoveConst<TFirst> First;
		VariantStorage_<TRest...> Rest;
	};

	constexpr VariantStorage_() noexcept {}
	INTRA_CONSTEXPR_DESTRUCTOR ~VariantStorage_() {}

	template<typename... Args>
	constexpr INTRA_FORCEINLINE explicit VariantStorage_(TIndex<0>, Args&&... args) noexcept(
		CNothrowConstructible<TFirst, TRest...>):
		First(static_cast<Args&&>(args)...) {}

	template<size_t I, typename... Args> requires(I > 0)
	constexpr INTRA_FORCEINLINE explicit VariantStorage_(TIndex<I>, Args&&... args) noexcept(
		CNothrowConstructible<VariantStorage_<TRest...>, TIndex<I - 1>, Args...>):
		Rest(TIndex<I - 1>{}, INTRA_FWD(args)...) {}

	VariantStorage_(VariantStorage_&&) = default;
	VariantStorage_(const VariantStorage_&) = default;
	VariantStorage_& operator=(VariantStorage_&&) = default;
	VariantStorage_& operator=(const VariantStorage_&) = default;
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
template<size_t I, typename T> requires CInstanceOfTemplate<TRemoveConstRef<T>, VariantStorage_>
constexpr decltype(auto) get(T&& vs) noexcept {return getStorage_<I>(INTRA_FWD(vs));}

template<typename... Ts> struct VariantImpl_: VariantStorage<Ts...>
{
	using IndexT = TSelect<uint8, int32, (sizeof...(Ts) < 256)>;
	IndexT mAlternativeIndex = sizeof...(Ts);

	VariantImpl_() noexcept: VariantStorage<Ts...>{} {}

	template<size_t I, typename... Args> requires CConstructible<TPackAt<I, Ts...>, Args...>
	constexpr explicit VariantImpl_(TConstructAt<I>, Args&&... args) noexcept(CNothrowConstructible<TPackAt<I, Ts...>, Args...>):
		VariantStorage_<Ts...>(TIndex<I>{}, INTRA_FWD(args)...),
		mAlternativeIndex(I) {}

	[[nodiscard]] constexpr bool Empty() const noexcept {return mAlternativeIndex == sizeof...(Ts);}
	[[nodiscard]] constexpr index_t CurrentAlternativeIndex() const noexcept {return mAlternativeIndex;}

	template<size_t I> void _Destroy() noexcept
	{
		INTRA_PRECONDITION(I == CurrentAlternativeIndex());
		if constexpr(I != sizeof...(Ts) && !CTriviallyDestructible<TPackAt<I, Ts...>>)
		{
			using T_I = TPackAt<I, Ts...>;
			getStorage_<I>(*this).~T_I();
		}
	}

	void _Destroy() noexcept
	{
		if constexpr(!(CTriviallyDestructible<Ts> && ...))
			if(!Empty()) ForFieldAtRuntime(mAlternativeIndex, []<typename T>(T& ref) noexcept {ref.~T();})(*this);
	}

	void _Reset() noexcept
	{
		_Destroy();
		mAlternativeIndex = sizeof...(Ts);
	}

	template<size_t I> void _Reset() noexcept
	{
		if constexpr(I != sizeof...(Ts))
		{
			_Destroy<I>();
			mAlternativeIndex = sizeof...(Ts);
		}
	}

	template<class VariantImpl> requires CSame<TUnqualRef<VariantImpl>, VariantImpl_>
	void _Construct_from(VariantImpl&& rhs) noexcept((CNothrowConstructible<Ts, TPropagateQualRef<VariantImpl, Ts>> && ...))
	{
		if(rhs.Empty()) return;
		ForFieldAtRuntime(rhs.mAlternativeIndex, [&, self=this]<size_t I, typename T>(T&& val, TIndex<I>) {
			using DstT = TUnqualRef<T>;
			DstT* storage = AddressOf(getStorage_<I>(*self));
			new(Construct, storage) DstT(INTRA_FWD(val));
		})(static_cast<TPropagateQualRef<decltype(rhs), VariantStorage<Ts...>>&>(rhs));
		mAlternativeIndex = rhs.mAlternativeIndex;
	}

	template<class VariantImpl> requires CSame<TUnqualRef<VariantImpl>, VariantImpl_>
	void _Assign_from(VariantImpl&& rhs) noexcept((
		(CNothrowConstructible<Ts, TPropagateQualRef<VariantImpl, Ts>> &&
			CNothrowAssignable<Ts, TPropagateQualRef<VariantImpl, Ts>>) && ...))
	{
		if(rhs.Empty())
		{
			_Reset();
			return;
		}
		ForFieldAtRuntime(rhs.mAlternativeIndex, [&, self=this]<size_t I, typename T>(T&& val, TIndex<I>) {
			using DstT = TUnqualRef<T>;
			DstT& storage = getStorage_<I>(*self);
			if(mAlternativeIndex == I)
			{
				storage = INTRA_FWD(val);
				return;
			}
			if constexpr(CRValueReference<T>)
			{
				_Reset();
				new(Construct, AddressOf(storage)) DstT(INTRA_MOVE(val));
			}
			else if constexpr(!CNothrowConstructible<DstT, T> && CNothrowMoveConstructible<DstT>)
			{
				DstT tempCopy = val;
				_Reset();
				new(Construct, AddressOf(storage)) DstT(INTRA_MOVE(tempCopy));
			}
			else
			{
				_Reset();
				new(Construct, AddressOf(storage)) DstT(val);
			}
			mAlternativeIndex = I;
		})(static_cast<TPropagateQualRef<decltype(rhs), VariantStorage<Ts...>>&>(rhs));
	}
};

template<typename... Ts> struct VariantImplWithDestructor_: VariantImpl_<Ts...>
{
	using VariantImpl_<Ts...>::VariantImpl_;

	INTRA_CONSTEXPR_DESTRUCTOR ~VariantImplWithDestructor_() noexcept
	{
		this->_Destroy();
	}

	VariantImplWithDestructor_() = default;
	VariantImplWithDestructor_(const VariantImplWithDestructor_&) = default;
	VariantImplWithDestructor_(VariantImplWithDestructor_&&) = default;
	VariantImplWithDestructor_& operator=(const VariantImplWithDestructor_&) = default;
	VariantImplWithDestructor_& operator=(VariantImplWithDestructor_&&) = default;
};

template<typename... Ts> using VariantImpl = TSelect<VariantImpl_<Ts...>, VariantImplWithDestructor_<Ts...>, (CTriviallyDestructible<Ts> && ...)>;

template<size_t I, typename T>
struct TVariantInitOverload_
{
	using F = TList<TIndex<I>, T>(*)(T);
	operator F();
};

template<class IndexSeq, typename... Ts>
struct TVariantInitOverloads_;

template<size_t... Is, typename... Ts>
struct TVariantInitOverloads_<TIndexSeq<Is...>, Ts...>: TVariantInitOverload_<Is, Ts>... {};

template<typename... Ts> using TVariantInitOverloads = TVariantInitOverloads_<TSeqFor<Ts...>, Ts...>;

INTRA_DEFINE_SAFE_DECLTYPE_T_ARGS(VariantInitHelper, TVariantInitOverloads<Args...>{}(Val<T>()));

template<class T, class... Ts> using VariantInitType = TListAt<1, VariantInitHelper<T, Ts...>>;
template<class T, class... Ts> using VariantInitIndex = TListAt<0, VariantInitHelper<T, Ts...>>;

}

template<typename... Ts> class Variant: private z_D::VariantImpl<Ts...>
{
	using Base = z_D::VariantImpl<Ts...>;
	static_assert(((CObject<Ts> && !CArray<Ts> && CDestructible<Ts>) && ...));
	static_assert(sizeof...(Ts) != 0);
public:
	constexpr Variant() noexcept(CNothrowConstructible<TPackFirst<Ts...>>) requires CConstructible<TPackFirst<Ts...>>: Base(ConstructAt<0>) {}

	constexpr Variant(const Variant& rhs): Base(other) {}
	constexpr Variant(Variant&& rhs) noexcept((CNothrowMoveConstructible<Ts> && ...)): Base(INTRA_MOVE(rhs)) {}

	template<typename T> requires (sizeof...(Ts) != 0) &&
		(!CSame<TUnqualRef<T>, Variant>) &&
		(!CInstanceOfTemplate<TUnqualRef<T>, TConstructT>) &&
		(!CConstructAt<TUnqualRef<T>>) &&
		CConstructible<z_D::VariantInitType<T, Ts...>, T>
	constexpr Variant(T&& t)
		noexcept(CNothrowConstructible<VariantInitType<T, Ts...>, T>):
		BASE(TConstructAt<VariantInitIndex<T, Ts...>::_>, INTRA_FWD(t)) {}

	template<typename T, typename... Args, size_t I = TListFindUnique<Variant, T>>
	requires (I != sizeof...(Ts)) && CConstructible<T, Args...>
	constexpr explicit Variant(TConstructT<T>, Args&&... args)
		noexcept(CNothrowConstructible<T, Args...>):
		BASE(TConstructAt<I>, INTRA_FWD(args)...) {}

	template<size_t I, typename... Args> requires CConstructible<TPackAt<I, Ts...>, Args...>
	constexpr explicit Variant(TConstructAt<I> at, Args&&... args)
		noexcept(CNothrowConstructible<TPackAt<I, Ts...>, Args...>):
		BASE(at, INTRA_FWD(args)...) {}

	template<typename T> requires
		(!CSame<TUnqualRef<T>, Variant>) &&
		CConstructible<z_D::VariantInitType<T, Ts...>, T> &&
		CAssignable<z_D::VariantInitType<T, Ts...>&, T>
	Variant& operator=(T&& rhs) noexcept(
		CNothrowAssignable<z_D::VariantInitType<T, Ts...>&, T> &&
			CNothrowConstructible<z_D::VariantInitType<T, Ts...>, T>)
	{
		constexpr size_t I = z_D::VariantInitIndex<T, Ts...>::_;
		if(mAlternativeIndex == I)
		{
			auto& storage = getStorage_<I>(static_cast<z_D::ValueStorage<Ts...>&>(*this));
			storage = INTRA_FWD(rhs);
			return *this;
		}
		using DstT = z_D::VariantInitType<T, Ts...>;
		if constexpr(!CNothrowConstructible<DstT, T> && CNothrowMoveConstructible<DstT>)
			Emplace<DstT>(DstT(INTRA_FWD(rhs)));
		else Emplace<DstT>(INTRA_FWD(rhs));
		return *this;
	}

	template<typename T, typename... Args, size_t I = TListFindUniqueIndex<Variant, T>> requires (I != sizeof...(Ts)) && CConstructible<T, Args...>
	T& Emplace(Args&&... args) noexcept(CNothrowConstructible<T, Args...>) {return Emplace<I>(INTRA_FWD(args)...);}

	template<size_t I, class... Args> requires CConstructible<TPackAt<I, Ts...>, Args...>
	TPackAt<I, Ts...>& Emplace(Args&&... args) noexcept(CNothrowConstructible<TPackAt<I, Ts...>, Args...>)
	{
		this->_Reset();
		auto& storage = getStorage_<I>(static_cast<z_D::ValueStorage<Ts...>&>(*this));
		new(Construct, AddressOf(storage)) TPackAt<I, Ts...>(INTRA_FWD(args)...));
		this->mAlternativeIndex = I;
		return storage;
	}
};

#ifdef INTRA_CONSTEXPR_TEST
static_assert(Variant<int, float, double>(3.14f) == Variant<int, float, double>(ConstructAt<1>, 3.14f));
#endif

INTRA_END
