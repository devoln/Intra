#pragma once

#include "Meta/Type.h"
#include "Cpp/Warnings.h"

INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS

namespace Intra {

namespace Meta {
INTRA_DEFINE_EXPRESSION_CHECKER2(HasOpEquals, Val<T1>() == Val<T2>(),, = U1);
INTRA_DEFINE_EXPRESSION_CHECKER2(HasOpNotEquals, Val<T1>() != Val<T2>(),, = U1);
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

INTRA_WARNING_POP
