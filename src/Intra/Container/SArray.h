#pragma once

#include "Intra/Assert.h"
#include "Intra/TypeSafe.h"

INTRA_BEGIN
template<typename T, size_t N> struct SArray
{
	T Elements[N];
	constexpr T* Data() noexcept {return Elements;}
	constexpr const T* Data() const noexcept {return Elements;}
	constexpr index_t Length() const noexcept {return N;}
	T& operator[](Index index) {INTRA_PRECONDITION(size_t(index) < N); return Elements[size_t(index)];}
	const T& operator[](Index index) const {INTRA_PRECONDITION(size_t(index) < N); return Elements[size_t(index)];}
};
template<typename T, typename... Ts> SArray(T, Ts...) ->
	SArray<RequiresAssert<CSame<T, Ts...>, T>, 1 + sizeof...(Ts)>;
INTRA_END
