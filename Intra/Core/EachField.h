#pragma once

#include "Core/Type.h"
#include "Core/Tuple.h"

INTRA_BEGIN
namespace D {
INTRA_DEFINE_CONCEPT_REQUIRES2(CHasForEachFieldMethod, Val<T1>().ForEachField(Val<T2>()), , = UniFunctor);
}
template<typename T, typename F=UniFunctor> concept CHasForEachFieldMethod =
	D::CHasForEachFieldMethod<TRemoveReference<T>, TRemoveReference<F>>;

template<typename Func, typename T> forceinline Requires<
	CHasForEachFieldMethod<T, Func>
> ForEachField(T&& value, Func&& f)
{value.ForEachField(Forward<Func>(f));}

namespace D {
INTRA_DEFINE_CONCEPT_REQUIRES2(CHasForEachField, ForEachField(Val<T1>(), Val<T2>()), , = UniFunctor);
}
template<typename T, typename F=UniFunctor> concept CHasForEachField =
	D::CHasForEachField<TRemoveReference<T>, TRemoveReference<F>>;
INTRA_END
