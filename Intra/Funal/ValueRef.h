#pragma once

#include "Cpp/Fundamental.h"
#include "Meta/Type.h"

INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS
INTRA_WARNING_DISABLE_COPY_MOVE_IMPLICITLY_DELETED

namespace Intra { namespace Funal {

template<typename T> struct TValue
{
	T Value;
	constexpr forceinline TValue(T&& value) noexcept: Value(Cpp::Move(value)) {}
	constexpr forceinline TValue(const T& value): Value(value) {}
	constexpr forceinline const T& operator()() const noexcept {return Value;}
	forceinline T& operator()() noexcept {return Value;}
};

template<typename T> constexpr forceinline TValue<Meta::RemoveConstRef<T>> Value(T&& val) {return Cpp::Forward<T>(val);}

template<typename T> struct TRef
{
	T& Ref;
	constexpr forceinline TRef(T& ref) noexcept: Ref(ref) {}
	constexpr forceinline T& operator()() const noexcept {return Ref;}
};

template<typename T> constexpr forceinline TRef<T> Ref(T& ref) {return ref;}

}}

INTRA_WARNING_POP
