#pragma once

#include "Intra/CContainer.h"

INTRA_BEGIN
template<typename C> constexpr Requires<
	CHas_empty<C>,
bool> operator==(const C& lhs, decltype(null)) {return lhs.empty();}


template<typename C> constexpr Requires<
	CHas_empty<C>,
bool> operator==(decltype(null), const C& rhs) {return rhs.empty();}

template<typename C> constexpr Requires<
	CHas_empty<C>,
bool> operator!=(const C& lhs, decltype(null)) {return !lhs.empty();}

template<typename C> constexpr Requires<
	CHas_empty<C>,
bool> operator!=(decltype(null), const C& rhs) {return !rhs.empty();}
INTRA_END
