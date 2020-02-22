#pragma once

#include "Type.h"
#include "Operations.h"
#include "CArray.h"
#include "Range/Concepts.h"

INTRA_BEGIN
INTRA_DEFINE_CONCEPT_REQUIRES(CHasPreIncrement, ++Val<T>());
INTRA_DEFINE_CONCEPT_REQUIRES(CHasPostIncrement, Val<T>()++);
INTRA_DEFINE_CONCEPT_REQUIRES(CHasPreDecrement, --Val<T>());
INTRA_DEFINE_CONCEPT_REQUIRES(CHasPostDecrement, Val<T>()--);
INTRA_DEFINE_CONCEPT_REQUIRES(CHasDereference, *Val<T>());
INTRA_DEFINE_CONCEPT_REQUIRES2(CHasDifference, static_cast<size_t>(Val<T1>()-Val<T2>()), , = U1);

template<typename T> concept CMinimalInputIterator=
	CHasPreIncrement<T> &&
	CHasDereference<T> &&
	CHasOpNotEquals<T, T> &&
	CMoveConstructible<T> &&
	CMoveAssignable<T> &&
	CDestructible<T>;

template<typename T> concept CInputIterator=
	CMinimalInputIterator<T> &&
	CHasPostIncrement<T> &&
	CEqualityComparable<T, T> &&
	CCopyConstructible<T> &&
	CCopyAssignable<T>;


namespace D {

template<typename R,
	bool = CHasDereference<TRemoveConstRef<R>>
> struct TIteratorReturnValueTypeOf2
{
	typedef TRemoveConstRef<R> RMut;
	typedef decltype(*Val<RMut>()) _;
};

template<typename R> struct TIteratorReturnValueTypeOf2<R, false>
{typedef void _;};

template<typename R> struct TIteratorReturnValueTypeOf: TIteratorReturnValueTypeOf2<R> {};
template<typename R> struct TIteratorValueTypeOf: TRemoveConstRef<typename TIteratorReturnValueTypeOf2<R>::_> {};

template<typename R,
	bool = CHas_begin_end<R>
> struct TIteratorOf
{typedef decltype(begin(Val<R>())) _;};

template<typename R> struct TIteratorOf<R, false> {typedef void _;};

}

template<typename R> using TIteratorReturnValueTypeOf = typename D::TIteratorReturnValueTypeOf<R>::_;
template<typename R> using TIteratorValueTypeOf = typename D::TIteratorValueTypeOf<R>::_;
template<typename R> using TIteratorOf = typename D::TIteratorOf<R>::_;


template<typename T> concept CMinimalBidirectionalIterator =
	CMinimalInputIterator<T> &&
	CHasPreDecrement<T> &&
	CCopyConstructible<T> &&
	CCopyAssignable<T>;
INTRA_END
