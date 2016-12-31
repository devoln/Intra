#pragma once

#include "Range/Concepts.h"
#include "Utils/Optional.h"

namespace Intra { namespace Range {

INTRA_WARNING_PUSH_DISABLE_COPY_MOVE_IMPLICITLY_DELETED

template<typename T, typename F> struct RRecurrence1
{
	enum: bool {RangeIsInfinite = true};

	forceinline RRecurrence1(null_t=null): func(), a(0) {}
	forceinline RRecurrence1(F function, T f1): func(function), a(f1) {}

	forceinline T First() const {return a;}
	forceinline void PopFirst() {a = func()(a);}
	forceinline bool Empty() const {return false;}

	forceinline bool operator==(const RRecurrence1<T, F>& rhs) const {return a==rhs.a;}
	forceinline bool operator!=(const RRecurrence1<T, F>& rhs) const {return a!=rhs.a;}

private:
	Utils::Optional<F> func;
	T a;
};

template<typename T, typename F> struct RRecurrence2
{
	enum: bool {RangeIsInfinite = true};

	forceinline RRecurrence2(null_t=null): func(), a(0), b(0) {}
	forceinline RRecurrence2(F function, T f1, T f2): func(function), a(f1), b(f2) {}

	forceinline T First() const {return a;}
	forceinline void PopFirst() {Meta::Swap(a, b); b = func()(b, a);}
	forceinline bool Empty() const {return false;}

	forceinline bool operator==(const RRecurrence2<T, F>& rhs) const {return a==rhs.a && b==rhs.b;}
	forceinline bool operator!=(const RRecurrence2<T, F>& rhs) const {return a!=rhs.a || b!=rhs.b;}

private:
	Utils::Optional<F> func;
	T a, b;
};

INTRA_WARNING_POP

template<typename T, typename F> forceinline
RRecurrence1<Meta::RemoveConstRef<T>, F> Recurrence(F function, T&& f1)
{return {function, Meta::Forward<T>(f1)};}


template<typename T, typename F> forceinline
RRecurrence2<Meta::RemoveConstRef<T>, F> Recurrence(F function, T&& f1, T&& f2)
{return {function, Meta::Forward<T>(f1), Meta::Forward<T>(f2)};}

}}
