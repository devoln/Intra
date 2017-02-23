#pragma once

#include "Platform/CppWarnings.h"
#include "Meta/Type.h"
#include "Range/Concepts.h"

namespace Intra {

namespace Range {template<typename T> struct ArrayRange;}

namespace Algo {

using namespace Range::Concepts;

INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS

//! Удаляет элементы из from, пока не получится to или from не станет пустым.
//! В результате либо from.Empty(), либо from==to.
//! Диапазон должен определять операцию сравнения на равенство.
//! \returns Количество удалённых элементов.
template<typename R> Meta::EnableIf<
	IsInputRange<R>::_ && !Meta::IsConst<R>::_ && !HasData<R>::_,
size_t> DistanceAdvanceTo(R& from, const R& to)
{
	size_t result = 0;
	while(!from.Empty() && !(from==to))
	{
		from.PopFirst();
		result++;
	}
	return result;
}

//! Удаляет элементы из from, пока не получится to или from не станет пустым.
//! В результате либо from.Empty(), либо from==to.
//! Диапазон должен определять операцию сравнения на равенство.
//! \returns Количество удалённых элементов.
template<typename R> Meta::EnableIf<
	IsInputRange<R>::_ && !Meta::IsConst<R>::_ && HasData<R>::_,
size_t> DistanceAdvanceTo(R& from, const R& to)
{
	size_t result = size_t(to.Data()-from.Data());
	from = to;
	return result;
}

//! Сколько элементов нужно удалить из from, пока не получится to или from не станет пустым.
//! Диапазон должен определять операцию сравнения на равенство.
//! \returns Количество удалённых элементов.
template<typename R> Meta::EnableIf<
	IsAccessibleRange<R>::_,
size_t> DistanceTo(R&& from, R&& to)
{
	auto fromCopy = Meta::Forward<R>(from);
	return DistanceAdvanceTo(fromCopy, to);
}

INTRA_WARNING_POP

}}
