#pragma once

#ifndef INTRA_LIBRARY_ATOMIC
#define INTRA_LIBRARY_ATOMIC INTRA_LIBRARY_ATOMIC_GNU
#endif

#include "Cpp/Features.h"
#include "Concurrency/Atomic.h"

namespace Intra { namespace Concurrency {

#define INTRA_ATOMIC_METHOD_GET(mo, MO) \
template<typename T> forceinline T AtomicBase<T>::Get ## mo() const noexcept \
{return __atomic_load_n(&mValue, __ATOMIC_ ## MO);}

#define INTRA_ATOMIC_METHOD_SET(mo, MO) \
template<typename T> forceinline void AtomicBase<T>::Set ## mo(T val) noexcept \
{__atomic_store_n(&mValue, val, __ATOMIC_ ## MO);}

#define INTRA_ATOMIC_METHOD_GET_SET(mo, MO) \
template<typename T> forceinline T AtomicBase<T>::GetSet ## mo(T val) noexcept \
{return __atomic_exchange_n(&mValue, val, __ATOMIC_ ## MO);}

#define INTRA_ATOMIC_METHOD_CAS(method, MO, weak) \
template<typename T> forceinline bool AtomicBase<T>::method(T& expected, T desired) noexcept \
{return __atomic_compare_exchange_n(&mValue, &expected, desired, weak, __ATOMIC_ ## MO, __ATOMIC_ ## MO);}

INTRA_ATOMIC_METHOD_GET(, SEQ_CST)
INTRA_ATOMIC_METHOD_GET(Relaxed, RELAXED)
INTRA_ATOMIC_METHOD_GET(Consume, CONSUME)
INTRA_ATOMIC_METHOD_GET(Acquire, ACQUIRE)

INTRA_ATOMIC_METHOD_SET(, SEQ_CST)
INTRA_ATOMIC_METHOD_SET(Relaxed, RELAXED)
INTRA_ATOMIC_METHOD_SET(Release, RELEASE)

INTRA_ATOMIC_METHOD_GET_SET(, SEQ_CST)
INTRA_ATOMIC_METHOD_GET_SET(Relaxed, RELAXED)
INTRA_ATOMIC_METHOD_GET_SET(Consume, CONSUME)
INTRA_ATOMIC_METHOD_GET_SET(Acquire, ACQUIRE)
INTRA_ATOMIC_METHOD_GET_SET(Release, RELEASE)
INTRA_ATOMIC_METHOD_GET_SET(AcquireRelease, ACQ_REL)

INTRA_ATOMIC_METHOD_CAS(WeakCompareSet, SEQ_CST, true)
INTRA_ATOMIC_METHOD_CAS(WeakCompareSetRelaxed, RELAXED, true)
INTRA_ATOMIC_METHOD_CAS(WeakCompareSetConsume, CONSUME, true)
INTRA_ATOMIC_METHOD_CAS(WeakCompareSetAcquire, ACQUIRE, true)
INTRA_ATOMIC_METHOD_CAS(WeakCompareSetRelease, RELEASE, true)
INTRA_ATOMIC_METHOD_CAS(WeakCompareSetAcquireRelease, ACQ_REL, true)

INTRA_ATOMIC_METHOD_CAS(CompareSet, SEQ_CST, false)
INTRA_ATOMIC_METHOD_CAS(CompareSetRelaxed, RELAXED, false)
INTRA_ATOMIC_METHOD_CAS(CompareSetConsume, CONSUME, false)
INTRA_ATOMIC_METHOD_CAS(CompareSetAcquire, ACQUIRE, false)
INTRA_ATOMIC_METHOD_CAS(CompareSetRelease, RELEASE, false)
INTRA_ATOMIC_METHOD_CAS(CompareSetAcquireRelease, ACQ_REL, false)

#define INTRA_ATOMIC_METHOD_BINARY_ONE(method, stdmethod, mo, MO) \
template<typename T> forceinline T AtomicInteger<T>::method ## mo(T rhs) noexcept \
{return __atomic_ ## stdmethod(&this->mValue, rhs, __ATOMIC_ ## MO);}

#define INTRA_ATOMIC_METHOD_BINARY(method, stdmethod) \
	INTRA_ATOMIC_METHOD_BINARY_ONE(method, stdmethod, , SEQ_CST) \
	INTRA_ATOMIC_METHOD_BINARY_ONE(method, stdmethod, Relaxed, RELAXED) \
	INTRA_ATOMIC_METHOD_BINARY_ONE(method, stdmethod, Consume, CONSUME) \
	INTRA_ATOMIC_METHOD_BINARY_ONE(method, stdmethod, Acquire, ACQUIRE) \
	INTRA_ATOMIC_METHOD_BINARY_ONE(method, stdmethod, Release, RELEASE) \
	INTRA_ATOMIC_METHOD_BINARY_ONE(method, stdmethod, AcquireRelease, ACQ_REL)

INTRA_ATOMIC_METHOD_BINARY(GetAdd, fetch_add)
INTRA_ATOMIC_METHOD_BINARY(GetSub, fetch_sub)
INTRA_ATOMIC_METHOD_BINARY(GetAnd, fetch_and)
INTRA_ATOMIC_METHOD_BINARY(GetOr, fetch_or)
INTRA_ATOMIC_METHOD_BINARY(GetXor, fetch_xor)

INTRA_ATOMIC_METHOD_BINARY(Add, add_fetch)
INTRA_ATOMIC_METHOD_BINARY(Sub, sub_fetch)
INTRA_ATOMIC_METHOD_BINARY(And, and_fetch)
INTRA_ATOMIC_METHOD_BINARY(Or, or_fetch)
INTRA_ATOMIC_METHOD_BINARY(Xor, xor_fetch)

#define INTRA_ATOMIC_INCREMENT_DECREMENT(mo, a) \
template<typename T> forceinline T AtomicInteger<T>::Increment ## mo() noexcept {return Add ## mo a(1);}\
template<typename T> forceinline T AtomicInteger<T>::Decrement ## mo() noexcept {return Sub ## mo a(1);}\
template<typename T> forceinline T AtomicInteger<T>::GetIncrement ## mo() noexcept {return GetAdd ## mo a(1);}\
template<typename T> forceinline T AtomicInteger<T>::GetDecrement ## mo() noexcept {return GetSub ## mo a(1);}

INTRA_ATOMIC_INCREMENT_DECREMENT(,)
INTRA_ATOMIC_INCREMENT_DECREMENT(Relaxed,)
INTRA_ATOMIC_INCREMENT_DECREMENT(Consume,)
INTRA_ATOMIC_INCREMENT_DECREMENT(Acquire,)
INTRA_ATOMIC_INCREMENT_DECREMENT(Release,)
INTRA_ATOMIC_INCREMENT_DECREMENT(AcquireRelease,)

#undef INTRA_ATOMIC_METHOD_GET
#undef INTRA_ATOMIC_METHOD_SET
#undef INTRA_ATOMIC_METHOD_GET_SET
#undef INTRA_ATOMIC_METHOD_CAS
#undef INTRA_ATOMIC_METHOD_BINARY_ONE
#undef INTRA_ATOMIC_METHOD_BINARY
#undef INTRA_ATOMIC_INCREMENT_DECREMENT

}}
