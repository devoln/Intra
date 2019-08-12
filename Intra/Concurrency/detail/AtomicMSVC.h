#pragma once

#ifndef INTRA_LIBRARY_ATOMIC
#define INTRA_LIBRARY_ATOMIC INTRA_LIBRARY_ATOMIC_MSVC
#endif

#include "Core/Type.h"
#include "Core/Preprocessor.h"
#include "Concurrency/Atomic.h"

#include <intrin.h>

INTRA_BEGIN
namespace Concurrency {

#if defined(_M_ARM) || defined(_M_ARM64)
//TODO: неизвестно, правильно ли это
#define MEMORY_BARRIER _ReadWriteBarrier

#define INTRIN_RELAXED_SUFFIX(...) INTRA_CONCATENATE_TOKENS(__VA_ARGS__, _nf)
#define INTRIN_ACQUIRE_SUFFIX(...) INTRA_CONCATENATE_TOKENS(__VA_ARGS__, _acq)
#define INTRIN_RELEASE_SUFFIX(...) INTRA_CONCATENATE_TOKENS(__VA_ARGS__, _rel)

#else

#define MEMORY_BARRIER _ReadWriteBarrier

#define INTRIN_RELAXED_SUFFIX(...) __VA_ARGS__
#define INTRIN_ACQUIRE_SUFFIX(...) __VA_ARGS__
#define INTRIN_RELEASE_SUFFIX(...) __VA_ARGS__

#endif

#if defined(_M_IX86)

#define INTERLOCKED_OP64(name, arg) \
inline int64 _Interlocked ## name ## 64(volatile int64* dst, int64 val) noexcept \
{\
	_ReadWriteBarrier();\
	int64 oldVal, newVal;\
	do {\
		oldVal = *dst;\
		newVal = val arg;\
	} while(oldVal != _InterlockedCompareExchange64(dst, newVal, oldVal));\
	_ReadWriteBarrier();\
	return oldVal;\
}

#ifndef _InterlockedExchange64
INTERLOCKED_OP64(Exchange,)
#endif

#ifndef _InterlockedExchangeAdd64
INTERLOCKED_OP64(ExchangeAdd, + oldVal)
#endif

#ifndef _InterlockedAnd64
INTERLOCKED_OP64(And, & oldVal)
#endif

#ifndef _InterlockedOr64
INTERLOCKED_OP64(Or, | oldVal)
#endif

#ifndef _InterlockedXor64
INTERLOCKED_OP64(Xor, ^ oldVal)
#endif

#endif

#if defined(_M_ARM) || defined(_M_ARM64)
template<> forceinline void AtomicBase<bool>::SetRelaxed(bool val) noexcept {__iso_volatile_store8(&mValue, val);}
template<> forceinline void AtomicBase<int>::SetRelaxed(int val) noexcept {__iso_volatile_store32(&mValue, val);}

#ifdef _M_ARM64
template<> forceinline void AtomicBase<int64>::SetRelaxed(int64 val) noexcept {__iso_volatile_store64(&mValue, val);}
#endif

#ifdef _M_ARM64
template<> forceinline void AtomicBase<int64>::SetRelease(int64 val) noexcept
{
	MEMORY_BARRIER();
	__iso_volatile_store64(&mValue, val);
}
#endif

template<> forceinline void AtomicBase<bool>::Set(bool val) noexcept
{
	MEMORY_BARRIER();
	SetRelaxed(val);
	MEMORY_BARRIER();
}

template<> forceinline void AtomicBase<int>::Set(int val) noexcept
{
	MEMORY_BARRIER();
	SetRelaxed(val);
	MEMORY_BARRIER();
}

#ifdef _M_ARM64
template<> forceinline void AtomicBase<int64>::Set(int64 val) noexcept
{
	MEMORY_BARRIER();
	__iso_volatile_store64(&mValue, val);
	MEMORY_BARRIER();
}
#else
template<> forceinline void AtomicBase<int64>::Set(int64 val) noexcept {GetSet(val);}
#endif


template<> forceinline bool AtomicBase<bool>::GetRelaxed() const noexcept
{return bool(__iso_volatile_load8(&mValue));}

template<> forceinline int AtomicBase<int>::GetRelaxed() const noexcept
{return __iso_volatile_load32(&mValue);}

#else

template<> forceinline void AtomicBase<bool>::SetRelaxed(bool val) noexcept {const_cast<volatile bool&>(mValue) = val;}
template<> forceinline void AtomicBase<int>::SetRelaxed(int val) noexcept {const_cast<volatile int&>(mValue) = val;}

#ifdef _M_X64
template<> forceinline void AtomicBase<int64>::SetRelaxed(int64 val) noexcept {const_cast<volatile int64&>(mValue) = val;}

template<> forceinline void AtomicBase<int64>::SetRelease(int64 val) noexcept
{
	MEMORY_BARRIER();
	const_cast<volatile int64&>(mValue) = val;
}
#endif

template<typename T> forceinline void AtomicBase<T>::Set(T val) noexcept {GetSet(val);}

template<> forceinline bool AtomicBase<bool>::GetRelaxed() const noexcept {return const_cast<volatile bool&>(mValue);}
template<> forceinline int AtomicBase<int>::GetRelaxed() const noexcept {return const_cast<volatile int&>(mValue);}


#endif


template<> forceinline void AtomicBase<bool>::SetRelease(bool val) noexcept
{
	MEMORY_BARRIER();
	SetRelaxed(val);
}

template<> forceinline void AtomicBase<int>::SetRelease(int val) noexcept
{
	MEMORY_BARRIER();
	SetRelaxed(val);
}

template<> forceinline bool AtomicBase<bool>::Get() const noexcept
{
	const bool result = GetRelaxed();
	MEMORY_BARRIER();
	return result;
}

template<> forceinline int AtomicBase<int>::Get() const noexcept
{
	const int result = GetRelaxed();
	MEMORY_BARRIER();
	return result;
}

template<> forceinline int64 AtomicBase<int64>::Get() const noexcept
{
#if defined(_M_X64)
	const int64 val = const_cast<volatile int64&>(mValue);
	MEMORY_BARRIER();
#elif defined(_M_ARM)
	const int64 val = __ldrexd(&mValue);
	MEMORY_BARRIER();
#elif defined(_M_ARM64)
	const int64 val = __iso_volatile_load64(&mValue);
	MEMORY_BARRIER();
#else
	const int64 val = _InterlockedOr64(const_cast<volatile int64*>(&mValue), 0);
#endif
	return val;
}

template<> forceinline int64 AtomicBase<int64>::GetRelaxed() const noexcept
{
#if defined(_M_X64)
	return const_cast<volatile int64&>(mValue);
#elif defined(_M_ARM)
	return __ldrexd(&mValue);
#elif defined(_M_ARM64)
	return __iso_volatile_load64(&mValue);
#else
	return _InterlockedOr64(const_cast<volatile int64*>(&mValue), 0);
#endif
}

template<typename T> forceinline T AtomicBase<T>::GetAcquire() const noexcept {return Get();}
template<typename T> forceinline T AtomicBase<T>::GetConsume() const noexcept {return Get();}


#define INTRA_ATOMIC_METHOD_GET_SET(type, size, mo, mosuf) \
template<> forceinline type AtomicBase<type>::GetSet ## mo(type val) noexcept \
{\
	auto result = INTRA_CONCATENATE_TOKENS(INTRA_CONCATENATE_TOKENS(_InterlockedExchange, size), mosuf)(AnyPtr(&mValue), val); \
	return reinterpret_cast<type&>(result); \
}

INTRA_ATOMIC_METHOD_GET_SET(bool, 8, , )
INTRA_ATOMIC_METHOD_GET_SET(int, , , )
INTRA_ATOMIC_METHOD_GET_SET(int64, 64, , )

INTRA_ATOMIC_METHOD_GET_SET(bool, 8, Relaxed, INTRIN_RELAXED_SUFFIX())
INTRA_ATOMIC_METHOD_GET_SET(int, , Relaxed, INTRIN_RELAXED_SUFFIX())
INTRA_ATOMIC_METHOD_GET_SET(int64, 64, Relaxed, INTRIN_RELAXED_SUFFIX())

INTRA_ATOMIC_METHOD_GET_SET(bool, 8, Acquire, INTRIN_ACQUIRE_SUFFIX())
INTRA_ATOMIC_METHOD_GET_SET(int, , Acquire, INTRIN_ACQUIRE_SUFFIX())
INTRA_ATOMIC_METHOD_GET_SET(int64, 64, Acquire, INTRIN_ACQUIRE_SUFFIX())

INTRA_ATOMIC_METHOD_GET_SET(bool, 8, Release, INTRIN_RELEASE_SUFFIX())
INTRA_ATOMIC_METHOD_GET_SET(int, , Release, INTRIN_RELEASE_SUFFIX())
INTRA_ATOMIC_METHOD_GET_SET(int64, 64, Release, INTRIN_RELEASE_SUFFIX())

template<typename T> forceinline T AtomicBase<T>::GetSetConsume(T val) noexcept {return GetSetAcquire(val);}
template<typename T> forceinline T AtomicBase<T>::GetSetAcquireRelease(T val) noexcept {return GetSet(val);}


#if !defined(_M_ARM64) && !defined(_M_X64)

template<> forceinline void AtomicBase<int64>::SetRelaxed(int64 val) noexcept {GetSetRelaxed(val);}
template<> forceinline void AtomicBase<int64>::SetRelease(int64 val) noexcept {GetSetRelease(val);}


#endif


template<typename T> forceinline bool AtomicBase<T>::WeakCompareSet(T expected, T desired) noexcept {return CompareSet(expected, desired);}
template<typename T> forceinline bool AtomicBase<T>::WeakCompareSetRelaxed(T expected, T desired) noexcept {return CompareSetRelaxed(expected, desired);}
template<typename T> forceinline bool AtomicBase<T>::WeakCompareSetConsume(T expected, T desired) noexcept {return CompareSetConsume(expected, desired);}
template<typename T> forceinline bool AtomicBase<T>::WeakCompareSetAcquire(T expected, T desired) noexcept {return CompareSetAcquire(expected, desired);}
template<typename T> forceinline bool AtomicBase<T>::WeakCompareSetRelease(T expected, T desired) noexcept {return CompareSetRelease(expected, desired);}
template<typename T> forceinline bool AtomicBase<T>::WeakCompareSetAcquireRelease(T expected, T desired) noexcept {return CompareSetAcquireRelease(expected, desired);}

template<typename T> forceinline bool AtomicBase<T>::WeakCompareGetSet(T& expected, T desired) noexcept {return CompareGetSet(expected, desired);}
template<typename T> forceinline bool AtomicBase<T>::WeakCompareGetSetRelaxed(T& expected, T desired) noexcept {return CompareGetSetRelaxed(expected, desired);}
template<typename T> forceinline bool AtomicBase<T>::WeakCompareGetSetConsume(T& expected, T desired) noexcept {return CompareGetSetConsume(expected, desired);}
template<typename T> forceinline bool AtomicBase<T>::WeakCompareGetSetAcquire(T& expected, T desired) noexcept {return CompareGetSetAcquire(expected, desired);}
template<typename T> forceinline bool AtomicBase<T>::WeakCompareGetSetRelease(T& expected, T desired) noexcept {return CompareGetSetRelease(expected, desired);}
template<typename T> forceinline bool AtomicBase<T>::WeakCompareGetSetAcquireRelease(T& expected, T desired) noexcept {return CompareGetSetAcquireRelease(expected, desired);}


#define INTRA_ATOMIC_CAS(type, size, mo, mosuf) \
template<> forceinline bool AtomicBase<type>::CompareSet ## mo(type expected, type desired) noexcept \
{ \
	const auto prev = INTRA_CONCATENATE_TOKENS(INTRA_CONCATENATE_TOKENS(_InterlockedCompareExchange, size), mosuf)(AnyPtr(&mValue), desired, expected); \
	return reinterpret_cast<const type&>(prev) == expected; \
}

#define INTRA_ATOMIC_CAS2(type, size, mo, mosuf) \
template<> forceinline bool AtomicBase<type>::CompareGetSet ## mo(type& expected, type desired) noexcept \
{ \
	const auto prev = INTRA_CONCATENATE_TOKENS(INTRA_CONCATENATE_TOKENS(_InterlockedCompareExchange, size), mosuf)(AnyPtr(&mValue), desired, expected); \
	if(reinterpret_cast<const type&>(prev) == expected) return true; \
	expected = reinterpret_cast<const type&>(prev); \
	return false; \
}

INTRA_ATOMIC_CAS(bool, 8, , )
INTRA_ATOMIC_CAS(int, , , )
INTRA_ATOMIC_CAS(int64, 64, , )

INTRA_ATOMIC_CAS(bool, 8, Relaxed, INTRIN_RELAXED_SUFFIX())
INTRA_ATOMIC_CAS(int, , Relaxed, INTRIN_RELAXED_SUFFIX())
INTRA_ATOMIC_CAS(int64, 64, Relaxed, INTRIN_RELAXED_SUFFIX())

INTRA_ATOMIC_CAS(bool, 8, Acquire, INTRIN_ACQUIRE_SUFFIX())
INTRA_ATOMIC_CAS(int, , Acquire, INTRIN_ACQUIRE_SUFFIX())
INTRA_ATOMIC_CAS(int64, 64, Acquire, INTRIN_ACQUIRE_SUFFIX())

INTRA_ATOMIC_CAS(bool, 8, Release, INTRIN_RELEASE_SUFFIX())
INTRA_ATOMIC_CAS(int, , Release, INTRIN_RELEASE_SUFFIX())
INTRA_ATOMIC_CAS(int64, 64, Release, INTRIN_RELEASE_SUFFIX())


INTRA_ATOMIC_CAS2(bool, 8, , )
INTRA_ATOMIC_CAS2(int, , , )
INTRA_ATOMIC_CAS2(int64, 64, , )

INTRA_ATOMIC_CAS2(bool, 8, Relaxed, INTRIN_RELAXED_SUFFIX())
INTRA_ATOMIC_CAS2(int, , Relaxed, INTRIN_RELAXED_SUFFIX())
INTRA_ATOMIC_CAS2(int64, 64, Relaxed, INTRIN_RELAXED_SUFFIX())

INTRA_ATOMIC_CAS2(bool, 8, Acquire, INTRIN_ACQUIRE_SUFFIX())
INTRA_ATOMIC_CAS2(int, , Acquire, INTRIN_ACQUIRE_SUFFIX())
INTRA_ATOMIC_CAS2(int64, 64, Acquire, INTRIN_ACQUIRE_SUFFIX())

INTRA_ATOMIC_CAS2(bool, 8, Release, INTRIN_RELEASE_SUFFIX())
INTRA_ATOMIC_CAS2(int, , Release, INTRIN_RELEASE_SUFFIX())
INTRA_ATOMIC_CAS2(int64, 64, Release, INTRIN_RELEASE_SUFFIX())

template<typename T> forceinline bool AtomicBase<T>::CompareSetAcquireRelease(T expected, T desired) noexcept {return CompareSet(expected, desired);}
template<typename T> forceinline bool AtomicBase<T>::CompareSetConsume(T expected, T desired) noexcept {return CompareSetAcquire(expected, desired);}

template<typename T> forceinline bool AtomicBase<T>::CompareGetSetAcquireRelease(T& expected, T desired) noexcept {return CompareGetSet(expected, desired);}
template<typename T> forceinline bool AtomicBase<T>::CompareGetSetConsume(T& expected, T desired) noexcept {return CompareGetSetAcquire(expected, desired);}


#define INTRA_ATOMIC_BINARY_ONE(type, op, mo, func, negation) \
template<> forceinline type AtomicInteger<type>::Get ## op ## mo(type rhs) noexcept \
{return func(AnyPtr(&mValue), negation rhs);}

#define INTRA_ATOMIC_BINARY(type, op, func, negation) \
INTRA_ATOMIC_BINARY_ONE(type, op, , func, negation) \
INTRA_ATOMIC_BINARY_ONE(type, op, Relaxed, INTRIN_RELAXED_SUFFIX(func), negation) \
INTRA_ATOMIC_BINARY_ONE(type, op, Acquire, INTRIN_ACQUIRE_SUFFIX(func), negation) \
INTRA_ATOMIC_BINARY_ONE(type, op, Consume, INTRIN_ACQUIRE_SUFFIX(func), negation) \
INTRA_ATOMIC_BINARY_ONE(type, op, Release, INTRIN_RELEASE_SUFFIX(func), negation) \
INTRA_ATOMIC_BINARY_ONE(type, op, AcquireRelease, func, negation)

INTRA_ATOMIC_BINARY(int, Add, _InterlockedExchangeAdd,)
INTRA_ATOMIC_BINARY(int, Sub, _InterlockedExchangeAdd, -)
INTRA_ATOMIC_BINARY(int, And, _InterlockedAnd,)
INTRA_ATOMIC_BINARY(int, Or, _InterlockedOr,)
INTRA_ATOMIC_BINARY(int, Xor, _InterlockedXor,)


INTRA_ATOMIC_BINARY(int64, Add, _InterlockedExchangeAdd64,)
INTRA_ATOMIC_BINARY(int64, Sub, _InterlockedExchangeAdd64, -)
INTRA_ATOMIC_BINARY(int64, And, _InterlockedAnd64,)
INTRA_ATOMIC_BINARY(int64, Or, _InterlockedOr64,)
INTRA_ATOMIC_BINARY(int64, Xor, _InterlockedXor64,)




#define INTRA_ATOMIC_POSTBINARY_ONE(opname, op, mo) \
template<typename T> forceinline T AtomicInteger<T>::opname ## mo(T rhs) noexcept {return Get ## opname ## mo(rhs) op rhs;}

#define INTRA_ATOMIC_POSTBINARY(opname, op) \
INTRA_ATOMIC_POSTBINARY_ONE(opname, op,) \
INTRA_ATOMIC_POSTBINARY_ONE(opname, op, Relaxed) \
INTRA_ATOMIC_POSTBINARY_ONE(opname, op, Consume) \
INTRA_ATOMIC_POSTBINARY_ONE(opname, op, Acquire) \
INTRA_ATOMIC_POSTBINARY_ONE(opname, op, Release) \
INTRA_ATOMIC_POSTBINARY_ONE(opname, op, AcquireRelease)

INTRA_ATOMIC_POSTBINARY(Add, +)
INTRA_ATOMIC_POSTBINARY(Sub, -)
INTRA_ATOMIC_POSTBINARY(And, &)
INTRA_ATOMIC_POSTBINARY(Or, |)
INTRA_ATOMIC_POSTBINARY(Xor, ^)


#define INTRA_ATOMIC_INC_DEC_ONE(type, size, mo, modef) \
template<> forceinline type AtomicInteger<type>::Increment ## mo() noexcept \
{return INTRA_CONCATENATE_TOKENS(INTRA_CONCATENATE_TOKENS(_InterlockedIncrement, size), modef)(AnyPtr(&mValue));} \
template<> forceinline type AtomicInteger<type>::Decrement ## mo() noexcept \
{return INTRA_CONCATENATE_TOKENS(INTRA_CONCATENATE_TOKENS(_InterlockedDecrement, size), modef)(AnyPtr(&mValue));} \
template<> forceinline type AtomicInteger<type>::GetIncrement ## mo() noexcept {return Increment ## mo() - 1;} \
template<> forceinline type AtomicInteger<type>::GetDecrement ## mo() noexcept {return Decrement ## mo() + 1;}

#define INTRA_ATOMIC_INC_DEC(type, size) \
INTRA_ATOMIC_INC_DEC_ONE(type, size, , ) \
INTRA_ATOMIC_INC_DEC_ONE(type, size, Relaxed, INTRIN_RELAXED_SUFFIX()) \
INTRA_ATOMIC_INC_DEC_ONE(type, size, Consume, INTRIN_ACQUIRE_SUFFIX()) \
INTRA_ATOMIC_INC_DEC_ONE(type, size, Acquire, INTRIN_ACQUIRE_SUFFIX()) \
INTRA_ATOMIC_INC_DEC_ONE(type, size, Release, INTRIN_RELEASE_SUFFIX()) \
INTRA_ATOMIC_INC_DEC_ONE(type, size, AcquireRelease, )

INTRA_ATOMIC_INC_DEC(int,)

#if defined(_M_ARM) || defined(_M_ARM64) || defined(_M_X64)
INTRA_ATOMIC_INC_DEC(int64, 64)
#else

#define INTRA_ATOMIC_INC_DEC_ADD_SUB_ONE(type, mo) \
template<> forceinline type AtomicInteger<type>::Increment ## mo() noexcept {return Add ## mo(1);} \
template<> forceinline type AtomicInteger<type>::Decrement ## mo() noexcept {return Sub ## mo(1);} \
template<> forceinline type AtomicInteger<type>::GetIncrement ## mo() noexcept {return GetAdd ## mo(1);} \
template<> forceinline type AtomicInteger<type>::GetDecrement ## mo() noexcept {return GetSub ## mo(1);}

INTRA_ATOMIC_INC_DEC_ADD_SUB_ONE(int64,)
INTRA_ATOMIC_INC_DEC_ADD_SUB_ONE(int64, Relaxed)
INTRA_ATOMIC_INC_DEC_ADD_SUB_ONE(int64, Consume)
INTRA_ATOMIC_INC_DEC_ADD_SUB_ONE(int64, Acquire)
INTRA_ATOMIC_INC_DEC_ADD_SUB_ONE(int64, Release)
INTRA_ATOMIC_INC_DEC_ADD_SUB_ONE(int64, AcquireRelease)

#undef INTRA_ATOMIC_INC_DEC_ADD_SUB_ONE

#endif


#undef INTRIN_ACQUIRE_SUFFIX
#undef INTRIN_RELEASE_SUFFIX
#undef INTRIN_RELAXED_SUFFIX

#undef INTRA_ATOMIC_METHOD_GET_SET
#undef INTRA_ATOMIC_CAS
#undef INTRA_ATOMIC_BINARY_ONE
#undef INTRA_ATOMIC_BINARY
#undef INTRA_ATOMIC_POSTBINARY_ONE
#undef INTRA_ATOMIC_POSTBINARY
#undef INTRA_ATOMIC_INC_DEC_ONE
#undef INTRA_ATOMIC_INC_DEC

}}
