#pragma once

#include "Core/CContainer.h"

INTRA_BEGIN
inline namespace Container {

template<typename C> constexpr forceinline Requires<
	CHas_empty<C>,
bool> operator==(const C& lhs, null_t) {return lhs.empty();}


template<typename C> constexpr forceinline Requires<
	CHas_empty<C>,
bool> operator==(null_t, const C& rhs) {return rhs.empty();}

template<typename C> constexpr forceinline Requires<
	CHas_empty<C>,
bool> operator!=(const C& lhs, null_t) {return !lhs.empty();}

template<typename C> constexpr forceinline Requires<
	CHas_empty<C>,
bool> operator!=(null_t, const C& rhs) {return !rhs.empty();}

}
INTRA_END
