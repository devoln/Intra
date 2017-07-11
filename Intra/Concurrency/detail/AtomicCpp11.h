#pragma once

#ifndef INTRA_LIBRARY_ATOMIC
#define INTRA_LIBRARY_ATOMIC INTRA_LIBRARY_ATOMIC_Cpp11
#endif

#include "Cpp/Features.h"
#include "Concurrency/Atomic.h"
#include <atomic>

namespace Intra { namespace Concurrency {

#define INTRA_ATOMIC_METHOD_GET(mo, stdmo) \
template<typename T> forceinline T AtomicBase<T>::Get ## mo() const noexcept \
{return mValue.load(std::memory_order_ ## stdmo);}

#define INTRA_ATOMIC_METHOD_SET(mo, stdmo) \
template<typename T> forceinline void AtomicBase<T>::Set ## mo(T val) noexcept \
{mValue.store(val, std::memory_order_ ## stdmo);}

#define INTRA_ATOMIC_METHOD_GET_SET(mo, stdmo) \
template<typename T> forceinline T AtomicBase<T>::GetSet ## mo(T val) noexcept \
{return mValue.exchange(val, std::memory_order_ ## stdmo);}

#define INTRA_ATOMIC_METHOD_CAS(method, stdmethod, mo, stdmo) \
template<typename T> forceinline bool AtomicBase<T>::method ## mo(T expected, T desired) noexcept \
{return mValue.stdmethod(expected, desired, std::memory_order_ ## stdmo);}

#define INTRA_ATOMIC_METHOD_CAS2(method, stdmethod, mo, stdmo) \
template<typename T> forceinline bool AtomicBase<T>::method ## mo(T& expected, T desired) noexcept \
{return mValue.stdmethod(expected, desired, std::memory_order_ ## stdmo);}

INTRA_ATOMIC_METHOD_GET(, seq_cst)
INTRA_ATOMIC_METHOD_GET(Relaxed, relaxed)
INTRA_ATOMIC_METHOD_GET(Consume, consume)
INTRA_ATOMIC_METHOD_GET(Acquire, acquire)

INTRA_ATOMIC_METHOD_SET(, seq_cst)
INTRA_ATOMIC_METHOD_SET(Relaxed, relaxed)
INTRA_ATOMIC_METHOD_SET(Release, release)

INTRA_ATOMIC_METHOD_GET_SET(, seq_cst)
INTRA_ATOMIC_METHOD_GET_SET(Relaxed, relaxed)
INTRA_ATOMIC_METHOD_GET_SET(Consume, consume)
INTRA_ATOMIC_METHOD_GET_SET(Acquire, acquire)
INTRA_ATOMIC_METHOD_GET_SET(Release, release)
INTRA_ATOMIC_METHOD_GET_SET(AcquireRelease, acq_rel)

INTRA_ATOMIC_METHOD_CAS(WeakCompareSet, compare_exchange_weak, , seq_cst)
INTRA_ATOMIC_METHOD_CAS(WeakCompareSet, compare_exchange_weak, Relaxed, relaxed)
INTRA_ATOMIC_METHOD_CAS(WeakCompareSet, compare_exchange_weak, Consume, consume)
INTRA_ATOMIC_METHOD_CAS(WeakCompareSet, compare_exchange_weak, Acquire, acquire)
INTRA_ATOMIC_METHOD_CAS(WeakCompareSet, compare_exchange_weak, Release, release)
INTRA_ATOMIC_METHOD_CAS(WeakCompareSet, compare_exchange_weak, AcquireRelease, acq_rel)

INTRA_ATOMIC_METHOD_CAS2(WeakCompareGetSet, compare_exchange_weak, , seq_cst)
INTRA_ATOMIC_METHOD_CAS2(WeakCompareGetSet, compare_exchange_weak, Relaxed, relaxed)
INTRA_ATOMIC_METHOD_CAS2(WeakCompareGetSet, compare_exchange_weak, Consume, consume)
INTRA_ATOMIC_METHOD_CAS2(WeakCompareGetSet, compare_exchange_weak, Acquire, acquire)
INTRA_ATOMIC_METHOD_CAS2(WeakCompareGetSet, compare_exchange_weak, Release, release)
INTRA_ATOMIC_METHOD_CAS2(WeakCompareGetSet, compare_exchange_weak, AcquireRelease, acq_rel)

INTRA_ATOMIC_METHOD_CAS(CompareSet, compare_exchange_strong, , seq_cst)
INTRA_ATOMIC_METHOD_CAS(CompareSet, compare_exchange_strong, Relaxed, relaxed)
INTRA_ATOMIC_METHOD_CAS(CompareSet, compare_exchange_strong, Consume, consume)
INTRA_ATOMIC_METHOD_CAS(CompareSet, compare_exchange_strong, Acquire, acquire)
INTRA_ATOMIC_METHOD_CAS(CompareSet, compare_exchange_strong, Release, release)
INTRA_ATOMIC_METHOD_CAS(CompareSet, compare_exchange_strong, AcquireRelease, acq_rel)

INTRA_ATOMIC_METHOD_CAS(CompareGetSet, compare_exchange_strong, , seq_cst)
INTRA_ATOMIC_METHOD_CAS(CompareGetSet, compare_exchange_strong, Relaxed, relaxed)
INTRA_ATOMIC_METHOD_CAS(CompareGetSet, compare_exchange_strong, Consume, consume)
INTRA_ATOMIC_METHOD_CAS(CompareGetSet, compare_exchange_strong, Acquire, acquire)
INTRA_ATOMIC_METHOD_CAS(CompareGetSet, compare_exchange_strong, Release, release)
INTRA_ATOMIC_METHOD_CAS(CompareGetSet, compare_exchange_strong, AcquireRelease, acq_rel)

#define INTRA_ATOMIC_METHOD_PREBINARY_ONE(method, stdmethod, mo, stdmo) \
template<typename T> forceinline T AtomicInteger<T>::method ## mo(T rhs) noexcept \
{return mValue.stdmethod(rhs, std::memory_order_ ## stdmo);}

#define INTRA_ATOMIC_METHOD_PREBINARY(method, stdmethod) \
	INTRA_ATOMIC_METHOD_PREBINARY_ONE(method, stdmethod, , seq_cst) \
	INTRA_ATOMIC_METHOD_PREBINARY_ONE(method, stdmethod, Relaxed, relaxed) \
	INTRA_ATOMIC_METHOD_PREBINARY_ONE(method, stdmethod, Consume, consume) \
	INTRA_ATOMIC_METHOD_PREBINARY_ONE(method, stdmethod, Acquire, acquire) \
	INTRA_ATOMIC_METHOD_PREBINARY_ONE(method, stdmethod, Release, release) \
	INTRA_ATOMIC_METHOD_PREBINARY_ONE(method, stdmethod, AcquireRelease, acq_rel)

INTRA_ATOMIC_METHOD_PREBINARY(GetAdd, fetch_add)
INTRA_ATOMIC_METHOD_PREBINARY(GetSub, fetch_sub)
INTRA_ATOMIC_METHOD_PREBINARY(GetAnd, fetch_and)
INTRA_ATOMIC_METHOD_PREBINARY(GetOr, fetch_or)
INTRA_ATOMIC_METHOD_PREBINARY(GetXor, fetch_xor)

#define INTRA_ATOMIC_METHOD_POSTBINARY_ONE(method, stdmethod, mo, stdmo, op) \
template<typename T> forceinline T AtomicInteger<T>::method ## mo(T rhs) noexcept \
{return mValue.stdmethod(rhs, std::memory_order_ ## stdmo) op rhs;}

#define INTRA_ATOMIC_METHOD_POSTBINARY(method, stdmethod, op) \
	INTRA_ATOMIC_METHOD_POSTBINARY_ONE(method, stdmethod, , seq_cst, op) \
	INTRA_ATOMIC_METHOD_POSTBINARY_ONE(method, stdmethod, Relaxed, relaxed, op) \
	INTRA_ATOMIC_METHOD_POSTBINARY_ONE(method, stdmethod, Consume, consume, op) \
	INTRA_ATOMIC_METHOD_POSTBINARY_ONE(method, stdmethod, Acquire, acquire, op) \
	INTRA_ATOMIC_METHOD_POSTBINARY_ONE(method, stdmethod, Release, release, op) \
	INTRA_ATOMIC_METHOD_POSTBINARY_ONE(method, stdmethod, AcquireRelease, acq_rel, op)

INTRA_ATOMIC_METHOD_POSTBINARY(Add, fetch_add, +)
INTRA_ATOMIC_METHOD_POSTBINARY(Sub, fetch_sub, -)
INTRA_ATOMIC_METHOD_POSTBINARY(And, fetch_and, &)
INTRA_ATOMIC_METHOD_POSTBINARY(Or, fetch_or, |)
INTRA_ATOMIC_METHOD_POSTBINARY(Xor, fetch_xor, ^)


template<typename T> forceinline T AtomicInteger<T>::Increment() noexcept {return ++mValue;}
template<typename T> forceinline T AtomicInteger<T>::Decrement() noexcept {return --mValue;}
template<typename T> forceinline T AtomicInteger<T>::GetIncrement() noexcept {return mValue++;}
template<typename T> forceinline T AtomicInteger<T>::GetDecrement() noexcept {return mValue--;}

#define INTRA_ATOMIC_INCREMENT_DECREMENT(mo) \
template<typename T> forceinline T AtomicInteger<T>::Increment ## mo() noexcept {return Add ## mo(1);}\
template<typename T> forceinline T AtomicInteger<T>::Decrement ## mo() noexcept {return Sub ## mo(1);}\
template<typename T> forceinline T AtomicInteger<T>::GetIncrement ## mo() noexcept {return GetAdd ## mo(1);}\
template<typename T> forceinline T AtomicInteger<T>::GetDecrement ## mo() noexcept {return GetSub ## mo(1);}

INTRA_ATOMIC_INCREMENT_DECREMENT(Relaxed)
INTRA_ATOMIC_INCREMENT_DECREMENT(Consume)
INTRA_ATOMIC_INCREMENT_DECREMENT(Acquire)
INTRA_ATOMIC_INCREMENT_DECREMENT(Release)
INTRA_ATOMIC_INCREMENT_DECREMENT(AcquireRelease)

#undef INTRA_ATOMIC_METHOD_GET
#undef INTRA_ATOMIC_METHOD_SET
#undef INTRA_ATOMIC_METHOD_GET_SET
#undef INTRA_ATOMIC_METHOD_CAS
#undef INTRA_ATOMIC_METHOD_PREBINARY_ONE
#undef INTRA_ATOMIC_METHOD_PREBINARY
#undef INTRA_ATOMIC_METHOD_POSTBINARY_ONE
#undef INTRA_ATOMIC_METHOD_POSTBINARY
#undef INTRA_ATOMIC_INCREMENT_DECREMENT

}}
