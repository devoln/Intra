#pragma once

#include "Meta/Type.h"
#include "Meta/Operators.h"

namespace Intra { namespace Range {

template<typename T> struct IsMinimalInputIterator: Meta::TypeFromValue<bool,
	Meta::HasPreIncrement<T>::_ &&
	Meta::HasDereference<T>::_ &&
	Meta::HasOpNotEquals<T, T>::_ &&
	Meta::IsMoveConstructible<T>::_ &&
	Meta::IsMoveAssignable<T>::_ &&
	Meta::IsDestructible<T>::_
> {};

template<typename T> struct IsInputIterator: Meta::TypeFromValue<bool,
	IsMinimalInputIterator<T>::_ &&
	Meta::HasPostIncrement<T>::_ &&
	Meta::HasOpEquals<T, T>::_ &&
	Meta::IsCopyConstructible<T>::_ &&
	Meta::IsCopyAssignable<T>::_
> {};


namespace RD {

template<typename R, bool=Meta::HasDereference<Meta::RemoveConstRef<R>>::_> struct IteratorReturnValueTypeOf2
{
	typedef Meta::RemoveConstRef<R> RMut;
	typedef decltype(*Meta::Val<RMut>()) _;
};

template<typename R> struct IteratorReturnValueTypeOf2<R, false>
{typedef void _;};

template<typename R> struct IteratorReturnValueTypeOf: IteratorReturnValueTypeOf2<R> {};
template<typename R> struct IteratorValueTypeOf: Meta::RemoveConstRef<typename IteratorReturnValueTypeOf2<R>::_> {};

}

template<typename R> using IteratorReturnValueTypeOf = typename RD::IteratorReturnValueTypeOf<R>::_;
template<typename R> using IteratorValueTypeOf = typename RD::IteratorValueTypeOf<R>::_;

}}
