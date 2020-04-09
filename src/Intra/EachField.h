#pragma once

#include "Intra/Type.h"
#include "Intra/Container/Tuple.h"

INTRA_BEGIN
namespace z_D {
INTRA_DEFINE_CONCEPT_REQUIRES2(CHasForEachFieldMethod, Val<T1>().ForEachField(Val<T2>()), , = UniFunctor);
}
template<typename T, typename F=UniFunctor> concept CHasForEachFieldMethod =
	z_D::CHasForEachFieldMethod<TRemoveReference<T>, TRemoveReference<F>>;

template<typename Func, typename T> constexpr Requires<
	CHasForEachFieldMethod<T, Func>
> ForEachField(T&& value, Func&& f)
{value.ForEachField(Forward<Func>(f));}

namespace z_D {
INTRA_DEFINE_CONCEPT_REQUIRES2(CHasForEachField, ForEachField(Val<T1>(), Val<T2>()), , = UniFunctor);
}
template<typename T, typename F=UniFunctor> concept CHasForEachField =
	z_D::CHasForEachField<TRemoveReference<T>, TRemoveReference<F>>;
INTRA_END
