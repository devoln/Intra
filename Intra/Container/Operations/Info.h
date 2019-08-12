#pragma once


#include "Core/Container.h"

INTRA_BEGIN
namespace Container {

INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS

template<typename C> forceinline Requires<
	CHas_empty<C>::_,
bool> operator==(const C& lhs, null_t) {return lhs.empty();}


template<typename C> forceinline Requires<
	CHas_empty<C>::_,
bool> operator==(null_t, const C& rhs) {return rhs.empty();}

template<typename C> forceinline Requires<
	CHas_empty<C>::_,
bool> operator!=(const C& lhs, null_t) {return !lhs.empty();}

template<typename C> forceinline Requires<
	CHas_empty<C>::_,
bool> operator!=(null_t, const C& rhs) {return !rhs.empty();}

INTRA_WARNING_POP

}}
