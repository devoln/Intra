#pragma once

#include "Meta/Type.h"
#include "Meta/Operators.h"
#include "Range.h"

namespace Intra { namespace Concepts {

INTRA_DEFINE_EXPRESSION_CHECKER(HasPreIncrement, ++Meta::Val<T>());
INTRA_DEFINE_EXPRESSION_CHECKER(HasPostIncrement, Meta::Val<T>()++);
INTRA_DEFINE_EXPRESSION_CHECKER(HasPreDecrement, --Meta::Val<T>());
INTRA_DEFINE_EXPRESSION_CHECKER(HasPostDecrement, Meta::Val<T>()--);
INTRA_DEFINE_EXPRESSION_CHECKER(HasDereference, *Meta::Val<T>());
INTRA_DEFINE_EXPRESSION_CHECKER2(HasDifference, static_cast<size_t>(Meta::Val<T1>()-Meta::Val<T2>()), , = U1);

template<typename T> struct IsMinimalInputIterator: Meta::TypeFromValue<bool,
	HasPreIncrement<T>::_ &&
	HasDereference<T>::_ &&
	Meta::HasOpNotEquals<T, T>::_ &&
	Meta::IsMoveConstructible<T>::_ &&
	Meta::IsMoveAssignable<T>::_ &&
	Meta::IsDestructible<T>::_
> {};

template<typename T> struct IsInputIterator: Meta::TypeFromValue<bool,
	IsMinimalInputIterator<T>::_ &&
	HasPostIncrement<T>::_ &&
	Meta::HasOpEquals<T, T>::_ &&
	Meta::IsCopyConstructible<T>::_ &&
	Meta::IsCopyAssignable<T>::_
> {};


namespace RD {

template<typename R, bool=HasDereference<Meta::RemoveConstRef<R>>::_> struct IteratorReturnValueTypeOf2
{
	typedef Meta::RemoveConstRef<R> RMut;
	typedef decltype(*Meta::Val<RMut>()) _;
};

template<typename R> struct IteratorReturnValueTypeOf2<R, false>
{typedef void _;};

template<typename R> struct IteratorReturnValueTypeOf: IteratorReturnValueTypeOf2<R> {};
template<typename R> struct IteratorValueTypeOf: Meta::RemoveConstRef<typename IteratorReturnValueTypeOf2<R>::_> {};

template<typename R, bool=Has_begin_end<R>::_> struct IteratorOf
{typedef decltype(begin(Meta::Val<R>())) _;};

template<typename R> struct IteratorOf<R, false> {typedef void _;};

}

template<typename R> using IteratorReturnValueTypeOf = typename RD::IteratorReturnValueTypeOf<R>::_;
template<typename R> using IteratorValueTypeOf = typename RD::IteratorValueTypeOf<R>::_;
template<typename R> using IteratorOf = typename RD::IteratorOf<R>::_;


template<typename T> struct IsMinimalBidirectionalIterator: Meta::TypeFromValue<bool,
	IsMinimalInputIterator<T>::_ &&
	HasPreDecrement<T>::_ &&
	Meta::IsCopyConstructible<T>::_ &&
	Meta::IsCopyAssignable<T>::_
> {};

}}
