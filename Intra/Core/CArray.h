#pragma once

#include "Core/Type.h"

INTRA_CORE_BEGIN
INTRA_DEFINE_CONCEPT_REQUIRES(CHas_size, static_cast<size_t>(Val<T>().size()));
INTRA_DEFINE_CONCEPT_REQUIRES(CHasLength, static_cast<size_t>(Val<T>().Length()));
INTRA_DEFINE_CONCEPT_REQUIRES(CHas_data, static_cast<const void*>(Val<T>().data()));
INTRA_DEFINE_CONCEPT_REQUIRES(CHasData, static_cast<const void*>(Val<T>().Data()));
INTRA_DEFINE_CONCEPT_REQUIRES(CHas_begin_end, Val<T>().begin() != Val<T>().end());

template<class R> using CHasLengthT = TBool<CHasLength<R>>;

namespace D {
template<typename R, bool = CHasData<R>> struct TReturnTypeOfData {typedef void _;};
template<typename R> struct TReturnTypeOfData<R, true> {typedef decltype(Val<R>().Data()) _;};

template<typename R, bool = CHasData<R>> struct TReturnTypeOfDataOrDisable;
template<typename R> struct TReturnTypeOfDataOrDisable<R, true> {typedef decltype(Val<R>().Data()) _;};

template<typename R, bool = CHas_data<R>> struct TReturnTypeOf_data {typedef void _;};
template<typename R> struct TReturnTypeOf_data<R, true> {typedef decltype(Val<R>().data()) _;};

template<typename R, bool = CHas_data<R>> struct TReturnTypeOf_data_OrDisable;
template<typename R> struct TReturnTypeOf_data_OrDisable<R, true> {typedef decltype(Val<R>().data()) _;};

}

template<typename T> using TReturnTypeOfData = typename D::TReturnTypeOfData<T>::_;
template<typename T> using TReturnTypeOfDataOrDisable = typename D::TReturnTypeOfDataOrDisable<T>::_;
template<typename T> using TReturnTypeOf_data = typename D::TReturnTypeOf_data<T>::_;
template<typename T> using TReturnTypeOf_data_OrDisable = typename D::TReturnTypeOf_data_OrDisable<T>::_;

template<typename R> constexpr forceinline Requires<
	CHasLength<R>,
size_t> LengthOf(const R& list) {return list.Length();}

template<typename R> constexpr forceinline Requires<
	CHas_size<R> &&
	!CHasLength<R>,
size_t> LengthOf(const R& list) {return list.size();}

template<typename T, size_t N> constexpr forceinline size_t LengthOf(T(&)[N]) {return N;}

template<typename R> constexpr forceinline TReturnTypeOfDataOrDisable<R> DataOf(R&& r) {return r.Data();}

template<typename R> constexpr forceinline Requires<
	!CHasData<R> &&
	!CHas_begin_end<R>,
TReturnTypeOf_data_OrDisable<R>> DataOf(R&& r) {return r.data();}

template<typename R, typename = Requires<
	!CHasData<R> &&
	CHas_data<R> &&
	CHas_begin_end<R>
>> constexpr forceinline decltype(&*begin(Val<R>())) DataOf(R&& r) {return &*begin(r);}

template<typename T> constexpr forceinline const T* DataOf(InitializerList<T> list) {return list.begin();}

template<typename T, size_t N> constexpr forceinline T* DataOf(T(&arr)[N]) {return arr;}

template<typename T> constexpr bool CInitializerList = false;
template<typename T> constexpr bool CInitializerList<InitializerList<T>> = true;

template<typename T> concept CHasDataOf =
	CHas_data<T> ||
	CHasData<T> ||
	CArrayType<TRemoveReference<T>> ||
	CInitializerList<T>;

template<typename T> concept CHasLengthOf =
	CHas_size<T> ||
	CHasLength<T> ||
	CArrayType<TRemoveReference<T>>;

template<typename R> concept CArrayClass =
	CHasDataOf<R> &&
	CHasLengthOf<R>;

namespace D {
template<typename R, int> struct TArrayElementPtr {typedef void _;};

template<typename R> struct TArrayElementPtr<R, 1> {typedef decltype(Val<R>().Data()) _;};
template<typename R> struct TArrayElementPtr<R, 2> {typedef decltype(&*begin(Val<R>())) _;};
template<typename R> struct TArrayElementPtr<R, 3> {typedef decltype(Val<R>().data()) _;};
template<typename T, size_t N> struct TArrayElementPtr<T(&)[N], 0> {typedef T* _;};
template<typename T> struct TArrayElementPtr<InitializerList<T>, 0> {typedef const T* _;};

template<typename R, int> struct TArrayElementPtrRequired;
template<typename R> struct TArrayElementPtrRequired<R, 1> {typedef decltype(Val<R>().Data()) _;};
template<typename R> struct TArrayElementPtrRequired<R, 2> {typedef decltype(&*begin(Val<R>())) _;};
template<typename R> struct TArrayElementPtrRequired<R, 3> {typedef decltype(Val<R>().data()) _;};
template<typename T, size_t N> struct TArrayElementPtrRequired<T(&)[N], 0> {typedef T* _;};
template<typename T, size_t N> struct TArrayElementPtrRequired<T[N], 0> {typedef T* _;};
template<typename T> struct TArrayElementPtrRequired<InitializerList<T>, 0> {typedef const T* _;};
}

template<typename R> using TArrayElementPtr = typename D::TArrayElementPtr<R, 
	CHasData<R>? 1:
		(CHas_data<R>?
			(CHas_begin_end<R>? 2: 3): 0)
>::_;

template<typename T> using TArrayElementRef = TRemovePointer<TArrayElementPtr<T>>&;
template<typename T> using TArrayElement = TRemoveConstPointer<TArrayElementPtr<T>>;
template<typename T> using TArrayElementKeepConst = TRemovePointer<TArrayElementPtr<T>>;

template<typename R> using TArrayElementPtrRequired = typename D::TArrayElementPtrRequired<R,
	CHasData<R>? 1:
		(CHas_data<R>?
			(CHas_begin_end<R>? 2: 3): 0)
>::_;

template<typename T> using TArrayElementRefRequired = TRemovePointer<TArrayElementPtrRequired<T>>&;
template<typename T> using TArrayElementRequired = TRemoveConstPointer<TArrayElementPtrRequired<T>>;
template<typename T> using TArrayElementKeepConstRequired = TRemovePointer<TArrayElementPtrRequired<T>>;

//! Проверяет, может ли массив Rhs быть присвоен массиву Lhs. Если это не array class, возвращает false.
template<typename Lhs, typename Rhs> concept CAssignableArrays =
	CSameNotVoid<
		TRemovePointer<TArrayElementPtr<Lhs>>,
		TRemovePointer<TArrayElementPtr<Rhs>>
	> ||
	CSameNotVoid<
		TRemovePointer<TArrayElementPtr<Lhs>>,
		TRemoveConstPointer<TArrayElementPtr<Rhs>>
	>;

template<typename Lhs, typename T> concept CAssignableToArrayOf =
	CSame<T, TRemovePointer<TArrayElementPtr<Lhs>>> ||
	CSame<TRemoveConst<T>, TRemovePointer<TArrayElementPtr<Lhs>>>;

template<typename R> concept CAssignableArrayClass =
	CArrayClass<R> &&
	!CConst<TRemovePointer<TArrayElementPtr<R>>>;

template<typename R> concept CTrivCopyableArray =
	CArrayClass<R> &&
	CTriviallyCopyable<TArrayElement<R>>;

template<typename R1, typename R2> concept CTrivCopyCompatibleArrayWith =
	CTrivCopyableArray<R1> &&
	CArrayClass<R2> &&
	CSameUnqual<TArrayElement<R1>, TArrayElement<R2>>;

template<typename T1, typename T2> concept CArrayClassOfExactly =
	CArrayClass<T1> &&
	CSameUnqual<TArrayElement<T1>, T2>;

#if INTRA_CONSTEXPR_TEST
static_assert(CArrayClass<const char(&)[5]>, "CArrayClass error.");
#endif
INTRA_CORE_END
