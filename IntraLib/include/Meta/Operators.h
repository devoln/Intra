#pragma once

#include "Meta/Type.h"

namespace Intra {

namespace Meta {
INTRA_DEFINE_EXPRESSION_CHECKER2(HasOpEquals, Meta::Val<T1>()==Meta::Val<T2>(),, = U1);
INTRA_DEFINE_EXPRESSION_CHECKER2(HasOpNotEquals, Meta::Val<T1>()!=Meta::Val<T2>(),, = U1);
}

template<typename T1, typename T2> Meta::EnableIf<
	Meta::HasOpEquals<T1, T2>::_ && !Meta::HasOpNotEquals<T1, T2>::_ &&
	(!Meta::IsScalarType<T1>::_ || !Meta::IsScalarType<T2>::_),
bool> operator!=(const T1& lhs, const T2& rhs)
{return !(lhs==rhs);}

template<typename T1, typename T2> Meta::EnableIf<
	Meta::HasOpNotEquals<T1, T2>::_ && !Meta::HasOpEquals<T1, T2>::_ &&
	(!Meta::IsScalarType<T1>::_ || !Meta::IsScalarType<T2>::_),
bool> operator==(const T1& lhs, const T2& rhs)
{return !(lhs!=rhs);}

}
