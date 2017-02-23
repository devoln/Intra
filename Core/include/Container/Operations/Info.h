#pragma once

#include "Platform/CppWarnings.h"
#include "Container/Concepts.h"

namespace Intra { namespace Container {

INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS

template<typename C, typename = Meta::EnableIf<
	Has_data<C>::_
>> forceinline decltype(&*Meta::Val<C>().begin()) Data(C&& v)
{
	typedef decltype(&*Meta::Val<C>().begin()) T;
	return const_cast<T>(v.data());
}

template<typename C> forceinline Meta::EnableIf<
	Has_empty<C>::_,
bool> operator==(const C& lhs, null_t) {return lhs.empty();}


template<typename C> forceinline Meta::EnableIf<
	Has_empty<C>::_,
bool> operator==(null_t, const C& rhs) {return rhs.empty();}

template<typename C> forceinline Meta::EnableIf<
	Has_empty<C>::_,
bool> operator!=(const C& lhs, null_t) {return !lhs.empty();}

template<typename C> forceinline Meta::EnableIf<
	Has_empty<C>::_,
bool> operator!=(null_t, const C& rhs) {return !rhs.empty();}

INTRA_WARNING_POP

}}
