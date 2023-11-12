#pragma once

#include <Intra/Core.h>

namespace Intra { INTRA_BEGIN

enum class MemoryOrder
{
	/// There are no synchronization or ordering constraints imposed on other reads or writes,
	/// only this operation's atomicity is guaranteed.
	Relaxed,

	/// No reads or writes in the current thread dependent on the value currently loaded can be reordered before this load.
	/// Writes to data-dependent variables in other threads that release the same atomic variable are visible in the current thread.
	Consume,

	/// No reads or writes in the current thread can be reordered before this load.
	/// All writes in other threads that release the same atomic variable are visible in the current thread.
	Acquire,

	/// No reads or writes in the current thread can be reordered after this store.
	/// All writes in the current thread are visible in other threads that acquire the same atomic variable
	/// and writes that carry a dependency into the atomic variable become visible in other threads that consume the same atomic.
	Release,

	/// No memory reads or writes in the current thread can be reordered before or after this store.
	/// All writes in other threads that release the same atomic variable are visible before the modification
	/// and the modification is visible in other threads that acquire the same atomic variable.
	AcquireRelease,

	/// Imposes all the constraints of the modes above, plus there is a single total order in which all threads observe all modifications.
	SequentiallyConsistent
};

#if !(defined(__GNUC__) || defined(__clang__))
//MSVC intrinsic declaration
namespace z_D { extern "C" {
void _ReadWriteBarrier();
#if defined(__arm__) || defined(__aarch64__)
int16 __iso_volatile_load16(const volatile int16*);
int32 __iso_volatile_load32(const volatile int32*);
int64 __iso_volatile_load64(const volatile int64*);
int8 __iso_volatile_load8(const volatile int8*);
void __iso_volatile_store16(volatile int16*, int16);
void __iso_volatile_store32(volatile int32*, int32);
void __iso_volatile_store64(volatile int64*, int64);
void __iso_volatile_store8(volatile int8*, int8);
#define INTRAZ_D_ARM_ONLY_CODE(...) __VA_ARGS__
#else
#define INTRAZ_D_ARM_ONLY_CODE(...)
#endif

#define INTRAZ_D_MEMORDER_REPLICATE(macro, ...) macro(__VA_ARGS__,) \
	INTRAZ_D_ARM_ONLY_CODE(macro(__VA_ARGS__, _acq) macro(__VA_ARGS__, _nf) macro(__VA_ARGS__, _rel))
#define INTRAZ_D_MEMORDER_AND_TYPE_REPLICATE(macro, ...) \
	INTRAZ_D_MEMORDER_REPLICATE(macro, int64, 64, __VA_ARGS__) \
	INTRAZ_D_MEMORDER_REPLICATE(macro, long, , __VA_ARGS__) \
	INTRAZ_D_MEMORDER_REPLICATE(macro, int16, 16, __VA_ARGS__) \
	INTRAZ_D_MEMORDER_REPLICATE(macro, char, 8, __VA_ARGS__)

#define INTRAZ_D_DECLARE_INTERLOCKED_OPS_ONE(T, bits, suffix) \
	T _InterlockedExchange##bits##suffix(volatile T* target, T value); \
	T _InterlockedCompareExchange##bits##suffix(volatile T* dst, T exchange, T comparand); \
	T _InterlockedAnd##bits##suffix(volatile T* val, T mask); \
	T _InterlockedOr##bits##suffix(volatile T* val, T mask); \
	T _InterlockedXor##bits##suffix(volatile T* val, T mask); \
	T _InterlockedExchangeAdd##bits##suffix(volatile T* addend, T value);
INTRAZ_D_MEMORDER_AND_TYPE_REPLICATE(INTRAZ_D_DECLARE_INTERLOCKED_OPS_ONE)
#undef INTRAZ_D_DECLARE_INTERLOCKED_OPS_ONE
#undef INTRAZ_D_MEMORDER_AND_TYPE_REPLICATE
#undef INTRAZ_D_MEMORDER_REPLICATE

#ifdef __i386__
#define INTRAZ_D_INTERLOCKED_OP64(name, arg) \
inline int64 _Interlocked ## name ## 64(volatile int64* dst, int64 val) noexcept \
{ \
	_ReadWriteBarrier(); \
	int64 oldVal, newVal; \
	do { \
		oldVal = *dst; \
		newVal = val arg; \
	} while(oldVal != _InterlockedCompareExchange64(dst, newVal, oldVal)); \
	_ReadWriteBarrier(); \
	return oldVal; \
}
INTRAZ_D_INTERLOCKED_OP64(Exchange,)
INTRAZ_D_INTERLOCKED_OP64(ExchangeAdd, + oldVal)
INTRAZ_D_INTERLOCKED_OP64(And, & oldVal)
INTRAZ_D_INTERLOCKED_OP64(Or, | oldVal)
INTRAZ_D_INTERLOCKED_OP64(Xor, ^ oldVal)
#undef INTRAZ_D_INTERLOCKED_OP64
#endif
}}
#endif

template<MemoryOrder memoryOrder = MemoryOrder::SequentiallyConsistent, typename T>
constexpr T AtomicLoad(const T* ptr) noexcept
{
	static_assert(memoryOrder != MemoryOrder::Release &&
		memoryOrder != MemoryOrder::AcquireRelease);
	if(!IsConstantEvaluated())
	{
	#if defined(__GNUC__) || defined(__clang__)
		return __atomic_load_n(ptr, int(memoryOrder));
	#elif defined(_MSC_VER)
		static_assert(sizeof(T) <= 8, "AtomicLoad is not implemented for this type yet.");
		TSignedIntOfSizeAtLeast<sizeof(T)> res;
		if constexpr(sizeof(T) == 8)
		{
		#if defined(__amd64__)
			res = *(const volatile int64*)ptr;
		#elif defined(__aarch64__)
			res = __iso_volatile_load64((const volatile int64*)ptr);
		#elif defined(__arm__)
			res = __ldrexd((const volatile int64*)ptr);
		#else
			res = _InterlockedOr64((const volatile int64*)ptr, 0);
			return reinterpret_cast<T&>(res); //return immediately since _InterlockedOr64 already has barriers in its implementation
		#endif
		}
		else
		{
		#if defined(__arm__) || defined(__aarch64__)
			if constexpr(sizeof(T) == 1) res = z_D::__iso_volatile_load8((const volatile int8*)ptr);
			else if constexpr(sizeof(T) == 2) res = z_D::__iso_volatile_load16((const volatile int16*)ptr);
			else if constexpr(sizeof(T) == 4) res = z_D::__iso_volatile_load32((const volatile int32*)ptr);
		#else
			res = *(const volatile decltype(res)*)ptr;
		#endif
		}
		if constexpr(memoryOrder != MemoryOrder::Relaxed) z_D::_ReadWriteBarrier();
		return reinterpret_cast<T&>(res);
	#else
		static_assert(false);
	#endif
	}
	return *ptr;
}

#define INTRAZ_D_MSVC_IMPL_REPLICATE_FOR_TYPES(templateMacro, suffix) \
	if constexpr(sizeof(T) == 1) templateMacro(8, suffix, char); \
	if constexpr(sizeof(T) == 2) templateMacro(16, suffix, short); \
	if constexpr(sizeof(T) == 4) templateMacro(, suffix, long); \
	if constexpr(sizeof(T) == 8) templateMacro(64, suffix, int64);

#define INTRAZ_D_MSVC_IMPL_REPLICATE_FOR_TYPES_AND_MEMORY_ORDERS(templateMacro) \
	INTRAZ_D_ARM_ONLY_CODE( \
		if constexpr(memoryOrder == MemoryOrder::Relaxed) {INTRAZ_D_MSVC_IMPL_REPLICATE_FOR_TYPES(templateMacro, _nf)} \
		else if constexpr(memoryOrder <= MemoryOrder::Acquire) {INTRAZ_D_MSVC_IMPL_REPLICATE_FOR_TYPES(templateMacro, _acq)} \
		else if constexpr(memoryOrder == MemoryOrder::Release) {INTRAZ_D_MSVC_IMPL_REPLICATE_FOR_TYPES(templateMacro, _rel)} \
		else ) \
	{INTRAZ_D_MSVC_IMPL_REPLICATE_FOR_TYPES(templateMacro,)}

template<MemoryOrder memoryOrder = MemoryOrder::SequentiallyConsistent, typename T>
constexpr T AtomicExchange(T* ptr, T val) noexcept
{
	static_assert(memoryOrder == MemoryOrder::Relaxed ||
		memoryOrder == MemoryOrder::SequentiallyConsistent ||
		memoryOrder == MemoryOrder::Acquire ||
		memoryOrder == MemoryOrder::Release ||
		memoryOrder == MemoryOrder::AcquireRelease);
	if(!IsConstantEvaluated())
	{
	#if defined(__GNUC__) || defined(__clang__)
		return __atomic_exchange_n(ptr, val, int(memoryOrder));
	#else
		static_assert(sizeof(T) <= 8, "AtomicExchange is not implemented for this type yet.");
		TSignedIntOfSizeAtLeast<sizeof(T)> res;
	#define INTRAZ_D_MSVC_IMPL_EXCHANGE(typeSize, suffix, T) res = _InterlockedExchange##typeSize##suffix(reinterpret_cast<T*>(ptr), reinterpret_cast<T&>(val))
		INTRAZ_D_MSVC_IMPL_REPLICATE_FOR_TYPES_AND_MEMORY_ORDERS(INTRAZ_D_MSVC_IMPL_EXCHANGE)
	#undef INTRAZ_D_MSVC_IMPL_EXCHANGE
	#endif
	}
	const T prev = *ptr;
	*ptr = val;
	return prev;
}

template<MemoryOrder memoryOrder = MemoryOrder::SequentiallyConsistent, typename T>
constexpr void AtomicStore(T* dst, T val) noexcept
{
	static_assert(memoryOrder == MemoryOrder::Relaxed ||
		memoryOrder == MemoryOrder::SequentiallyConsistent ||
		memoryOrder == MemoryOrder::Release);
	if(!IsConstantEvaluated())
	{
	#if defined(__GNUC__) || defined(__clang__)
		__atomic_store_n(dst, val, int(memoryOrder));
	#else
		static_assert(sizeof(T) <= 8, "AtomicStore is not implemented for this type yet.");
		if constexpr(memoryOrder >= MemoryOrder::Release) z_D::_ReadWriteBarrier();
		if constexpr(sizeof(T) == 8)
		{
		#if defined(__amd64__)
			*(const volatile int64*)ptr = reinterpret_cast<int64&>(val);
		#elif defined(__aarch64__)
			z_D::__iso_volatile_store64((volatile int64*)dst, reinterpret_cast<int64&>(val));
		#else
			AtomicExchange(dst, val);
			return;
		#endif
		}
		else
		{
		#if defined(__arm__) || defined(__aarch64__)
			if constexpr(sizeof(T) == 1) z_D::__iso_volatile_store8((volatile int8*)dst, reinterpret_cast<int8&>(val));
			else if constexpr(sizeof(T) == 2) z_D::__iso_volatile_store16((volatile int16*)dst, reinterpret_cast<int16&>(val));
			else if constexpr(sizeof(T) == 4) z_D::__iso_volatile_store32((volatile int32*)dst, reinterpret_cast<int32&>(val));
		#else
			*dst = val;
		#endif
		}
		if constexpr(memoryOrder == MemoryOrder::SequentiallyConsistent) z_D::_ReadWriteBarrier();
	#endif
		return;
	}
	*dst = val;
}

template<typename T> constexpr bool CompareExchange(T* ptr, T* expected, T desired) noexcept
{
	if(*ptr == *expected)
	{
		*ptr = desired;
		return true;
	}
	*expected = *ptr;
	return false;
}

template<MemoryOrder successMemoryOrder = MemoryOrder::SequentiallyConsistent,
	MemoryOrder failureMemoryOrder = MemoryOrder::SequentiallyConsistent,
	bool allowSpuriousFailure = false,
	typename T>
constexpr bool AtomicCompareExchange(T* ptr, T* expected, T desired) noexcept
{
	static_assert(successMemoryOrder <= failureMemoryOrder);
	static_assert(failureMemoryOrder != MemoryOrder::Release && failureMemoryOrder != MemoryOrder::AcquireRelease);
	if(!IsConstantEvaluated())
	{
	#if defined(__GNUC__) || defined(__clang__)
		return __atomic_compare_exchange_n(ptr, expected, desired, allowSpuriousFailure, int(successMemoryOrder), int(failureMemoryOrder));
	#else
		using TypeForICE = TSelect<long,
			TSelect<char, T, sizeof(T) == 1>,
			sizeof(T) == 4>;
		TypeForICE prev;
	#define INTRAZ_D_MSVC_IMPL_COMPARE_EXCHANGE(typeSize, suffix, T) prev = _InterlockedCompareExchange##typeSize##suffix(reinterpret_cast<T*>(ptr), reinterpret_cast<T&>(desired), *reinterpret_cast<T*>(expected))
		INTRAZ_D_MSVC_IMPL_REPLICATE_FOR_TYPES_AND_MEMORY_ORDERS(INTRAZ_D_MSVC_IMPL_COMPARE_EXCHANGE)
	#undef INTRAZ_D_MSVC_IMPL_COMPARE_EXCHANGE
		if(reinterpret_cast<T&>(prev) == expected) return true;
		*expected = reinterpret_cast<T&>(prev);
		return false;
	#endif
	}
	return CompareExchange(ptr, expected, desired);
}

template<MemoryOrder memoryOrder = MemoryOrder::SequentiallyConsistent, typename T, typename T2>
constexpr T AtomicFetchAdd(T* ptr, T2 val)
{
	if(!IsConstantEvaluated())
	{
	#if defined(__GNUC__) || defined(__clang__)
		return __atomic_fetch_add(ptr, val, int(memoryOrder));
	#else
	#define INTRAZ_D_MSVC_IMPL_ADD(typeSize, suffix, type) return _InterlockedExchangeAdd##typeSize##suffix(reinterpret_cast<type*>(ptr), type(val))
		INTRAZ_D_MSVC_IMPL_REPLICATE_FOR_TYPES_AND_MEMORY_ORDERS(INTRAZ_D_MSVC_IMPL_ADD)
	#undef INTRAZ_D_MSVC_IMPL_ADD
	#endif
	}
	if constexpr(CBasicPointer<T>)
	{
		INTRA_ASSERT(val % sizeof(**ptr) == 0); // fractional pointer offsets are not allowed in constexpr
		val /= sizeof(**ptr);
	}
	const T prev = *ptr;
	*ptr += val;
	return prev;
}

template<MemoryOrder memoryOrder = MemoryOrder::SequentiallyConsistent, typename T, typename T2>
constexpr T AtomicFetchSub(T* ptr, T2 val)
{
	if(!IsConstantEvaluated())
	{
	#if defined(__GNUC__) || defined(__clang__)
		return __atomic_fetch_sub(ptr, val, int(memoryOrder));
	#else
	#define INTRAZ_D_MSVC_IMPL_SUB(typeSize, suffix, T) return _InterlockedExchangeAdd##typeSize##suffix(reinterpret_cast<T*>(ptr), -reinterpret_cast<T&>(val))
		INTRAZ_D_MSVC_IMPL_REPLICATE_FOR_TYPES_AND_MEMORY_ORDERS(INTRAZ_D_MSVC_IMPL_SUB)
	#undef INTRAZ_D_MSVC_IMPL_SUB
	#endif
	}
	if constexpr(CBasicPointer<T>)
	{
		INTRA_ASSERT(val % sizeof(**ptr) == 0); // fractional pointer offsets are not allowed in constexpr
		val /= sizeof(*ptr);
	}
	const T prev = *ptr;
	*ptr -= val;
	return prev;
}

template<MemoryOrder memoryOrder = MemoryOrder::SequentiallyConsistent, typename T>
constexpr T AtomicFetchAnd(T* ptr, T val)
{
	if(!IsConstantEvaluated())
	{
	#if defined(__GNUC__) || defined(__clang__)
		return __atomic_fetch_and(ptr, val, int(memoryOrder));
	#else
	#define INTRAZ_D_MSVC_IMPL_AND(typeSize, suffix, T) return _InterlockedAnd##typeSize##suffix(reinterpret_cast<T*>(ptr), reinterpret_cast<T&>(val))
		INTRAZ_D_MSVC_IMPL_REPLICATE_FOR_TYPES_AND_MEMORY_ORDERS(INTRAZ_D_MSVC_IMPL_AND)
	#undef INTRAZ_D_MSVC_IMPL_AND
	#endif
	}
	if constexpr(CBasicIntegral<T>)
	{
		const T prev = *ptr;
		*ptr &= val;
		return prev;
	}
	else INTRA_ASSERT(CBasicIntegral<T>); // constexpr execution of this function supports only integers
}

template<MemoryOrder memoryOrder = MemoryOrder::SequentiallyConsistent, typename T>
constexpr T AtomicFetchOr(T* ptr, T val)
{
	if(!IsConstantEvaluated())
	{
	#if defined(__GNUC__) || defined(__clang__)
		return __atomic_fetch_or(ptr, val, int(memoryOrder));
	#else
	#define INTRAZ_D_MSVC_IMPL_OR(typeSize, suffix, T) return _InterlockedOr##typeSize##suffix(reinterpret_cast<T*>(ptr), reinterpret_cast<T&>(val))
		INTRAZ_D_MSVC_IMPL_REPLICATE_FOR_TYPES_AND_MEMORY_ORDERS(INTRAZ_D_MSVC_IMPL_OR)
	#undef INTRAZ_D_MSVC_IMPL_OR
	#endif
	}
	if constexpr(CBasicIntegral<T>)
	{
		const T prev = *ptr;
		*ptr |= val;
		return prev;
	}
	else INTRA_ASSERT(CBasicIntegral<T>); // constexpr execution of this function supports only integers
}

template<MemoryOrder memoryOrder = MemoryOrder::SequentiallyConsistent, typename T>
constexpr T AtomicFetchXor(T* ptr, T val)
{
	if(!IsConstantEvaluated())
	{
	#if defined(__GNUC__) || defined(__clang__)
		return __atomic_fetch_xor(ptr, val, int(memoryOrder));
	#else
	#define INTRAZ_D_MSVC_IMPL_XOR(typeSize, suffix, T) return _InterlockedXor##typeSize##suffix(reinterpret_cast<T*>(ptr), reinterpret_cast<T&>(val))
		INTRAZ_D_MSVC_IMPL_REPLICATE_FOR_TYPES_AND_MEMORY_ORDERS(INTRAZ_D_MSVC_IMPL_XOR)
	#undef INTRAZ_D_MSVC_IMPL_XOR
	#endif
	}
	
	if constexpr(CBasicIntegral<T>)
	{
		const T prev = *ptr;
		*ptr ^= val;
		return prev;
	}
	else INTRA_ASSERT(CBasicIntegral<T>); // constexpr execution of this function supports only integers
}

#undef INTRAZ_D_MSVC_IMPL_REPLICATE_FOR_TYPES_AND_MEMORY_ORDERS
#undef INTRAZ_D_MSVC_IMPL_REPLICATE_FOR_TYPES
#undef INTRAZ_D_ARM_ONLY_CODE


template<MemoryOrder memoryOrder = MemoryOrder::SequentiallyConsistent, typename T, typename T2>
constexpr T AtomicAddFetch(T* ptr, T2 val)
{
	if(!IsConstantEvaluated())
	{
	#if defined(__GNUC__) || defined(__clang__)
		return __atomic_add_fetch(ptr, val, int(memoryOrder));
	#else
		return AtomicFetchAdd<memoryOrder>(ptr, val) + val;
	#endif
	}
	if constexpr(CBasicPointer<T>)
	{
		INTRA_ASSERT(val % sizeof(**ptr) == 0); // fractional pointer offsets are not allowed in constexpr
		val /= sizeof(*ptr);
	}
	return *ptr += val;
}

template<MemoryOrder memoryOrder = MemoryOrder::SequentiallyConsistent, typename T, typename T2>
constexpr T AtomicSubFetch(T* ptr, T2 val)
{
	if(!IsConstantEvaluated())
	{
	#if defined(__GNUC__) || defined(__clang__)
		return __atomic_sub_fetch(ptr, val, int(memoryOrder));
	#else
		return AtomicFetchSub<memoryOrder>(ptr, val) - val;
	#endif
	}
	if constexpr(CBasicPointer<T>)
	{
		INTRA_ASSERT(val % sizeof(**ptr) == 0); // fractional pointer offsets are not allowed in constexpr
		val /= sizeof(*ptr);
	}
	return *ptr -= val;
}

template<MemoryOrder memoryOrder = MemoryOrder::SequentiallyConsistent, typename T>
constexpr T AtomicAndFetch(T* ptr, T val)
{
	if(!IsConstantEvaluated())
	{
	#if defined(__GNUC__) || defined(__clang__)
		return __atomic_and_fetch(ptr, val, int(memoryOrder));
	#else
		return AtomicFetchAnd<memoryOrder>(ptr, val) & val;
	#endif
	}
	if constexpr(CBasicIntegral<T>) return *ptr &= val;
	else INTRA_ASSERT(CBasicIntegral<T>); // constexpr execution of this function supports only integers
}

template<MemoryOrder memoryOrder = MemoryOrder::SequentiallyConsistent, typename T>
constexpr T AtomicOrFetch(T* ptr, T val)
{
	if(!IsConstantEvaluated())
	{
	#if defined(__GNUC__) || defined(__clang__)
		return __atomic_or_fetch(ptr, val, int(memoryOrder));
	#else
		return AtomicFetchOr<memoryOrder>(ptr, val) | val;
	#endif
	}
	if constexpr(CBasicIntegral<T>) return *ptr |= val;
	else INTRA_ASSERT(CBasicIntegral<T>); // constexpr execution of this function supports only integers
}

template<MemoryOrder memoryOrder = MemoryOrder::SequentiallyConsistent, typename T>
constexpr T AtomicXorFetch(T* ptr, T val)
{
	if(!IsConstantEvaluated())
	{
	#if defined(__GNUC__) || defined(__clang__)
		return __atomic_xor_fetch(ptr, val, int(memoryOrder));
	#else
		return AtomicFetchXor<memoryOrder>(ptr, val) ^ val;
	#endif
	}
	if constexpr(CBasicIntegral<T>) return *ptr ^= val;
	else INTRA_ASSERT(CBasicIntegral<T>); // constexpr execution of this function supports only integers
}

template<MemoryOrder memoryOrder = MemoryOrder::SequentiallyConsistent, typename T>
constexpr bool AtomicFlagGetSet(T* ptr)
{
	static_assert(CAnyOf<T, bool, char, sbyte, byte>);
	if(!IsConstantEvaluated())
	{
	#if defined(__GNUC__) || defined(__clang__)
		return __atomic_test_and_set(ptr, int(memoryOrder));
	#else
		return !!AtomicExchange<memoryOrder>(ptr, true);
	#endif
	}
	const bool res = !!*ptr;
	*ptr = true;
	return res;
}

template<MemoryOrder memoryOrder = MemoryOrder::SequentiallyConsistent, typename T>
constexpr void AtomicFlagReset(T* ptr)
{
	static_assert(CAnyOf<T, bool, char, sbyte, byte>);
	static_assert(memoryOrder != MemoryOrder::Consume &&
		memoryOrder != MemoryOrder::Acquire &&
		memoryOrder != MemoryOrder::AcquireRelease);
	if(!IsConstantEvaluated(ptr, *ptr, val))
	{
	#if defined(__GNUC__) || defined(__clang__)
		return __atomic_clear(ptr, int(memoryOrder));
	#else
		return AtomicStore<memoryOrder>(ptr, false);
	#endif
	}
	*ptr = false;
}

template<typename T> class Atomic
{
	Atomic(const Atomic&) = delete;
	Atomic& operator=(const Atomic&) = delete;
public:
	constexpr Atomic() noexcept: mValue{} {}
	constexpr Atomic(T val) noexcept: mValue{val} {}

	/// @return current value read atomically with corresponding memory ordering.
	template<MemoryOrder mo = MemoryOrder::SequentiallyConsistent>
	constexpr T Get() const noexcept {return AtomicLoad<mo>(&mValue);}
	
	/// Atomically set a new value with corresponding memory ordering.
	/// @return the previous value.
	template<MemoryOrder mo = MemoryOrder::SequentiallyConsistent>
	constexpr T GetSet(T val) noexcept {return AtomicExchange<mo>(&mValue, val);}

	/// Atomically change the flag state to true with corresponding memory ordering.
	/// This operation is always lock-free on any target architecture.
	/// @return the previous value of the flag.
	template<MemoryOrder mo = MemoryOrder::SequentiallyConsistent>
	constexpr bool GetSet() noexcept requires CSame<T, bool> {return AtomicTestAndSet<mo>(&mValue);}
	
	/// Set a new value with corresponding memory ordering.
	template<MemoryOrder mo = MemoryOrder::SequentiallyConsistent>
	constexpr void Set(T val) noexcept {AtomicStore<mo>(&mValue, val);}

	/// Resets the flag atomically with corresponding memory ordering.
	/// This operation is always lock-free on any target architecture.
	template<MemoryOrder mo = MemoryOrder::SequentiallyConsistent>
	constexpr void Reset() noexcept requires CSame<T, bool> {return AtomicFlagReset<mo>(&mValue);}

	constexpr operator bool() requires CSame<T, bool> {return Get();}

	constexpr bool IsAlwaysLockFree =
	#if defined(__GNUC__) || defined(__clang__)
		__atomic_always_lock_free(sizeof(T), nullptr);
	#elif defined(__amd64__)
		sizeof(T) <= 8;
	#else
		sizeof(T) <= sizeof(void*);
	#endif

	/// Atomically set value desired if the current value equals to expected.
	/// It may spuriously fail by acting as if *this != expected so it should be used in a loop.
	template<MemoryOrder successMemoryOrder = MemoryOrder::SequentiallyConsistent, MemoryOrder failureMemoryOrder = MemoryOrder::SequentiallyConsistent>
	constexpr bool WeakCompareSet(T expected, T desired) noexcept {return AtomicCompareExchange<successMemoryOrder, failureMemoryOrder, true>(&mValue, &expected, desired);}

	/// Atomically set value desired if the current value equals to expected, otherwise set expected to the current value.
	/// It may spuriously fail by acting as if *this != expected so it should be used in a loop.
	template<MemoryOrder successMemoryOrder = MemoryOrder::SequentiallyConsistent, MemoryOrder failureMemoryOrder = MemoryOrder::SequentiallyConsistent>
	constexpr bool WeakCompareGetSet(T& expected, T desired) noexcept {return AtomicCompareExchange<successMemoryOrder, failureMemoryOrder, true>(&mValue, &expected, desired);}

	/// Atomically set value desired if the current value equals to expected.
	/// On some platforms it may be slower than WeakCompareSet.
	template<MemoryOrder successMemoryOrder = MemoryOrder::SequentiallyConsistent, MemoryOrder failureMemoryOrder = MemoryOrder::SequentiallyConsistent>
	constexpr bool CompareSet(T expected, T desired) noexcept {return AtomicCompareExchange<successMemoryOrder, failureMemoryOrder, false>(&mValue, &expected, desired);}

	/// Atomically set value `desired` if the current value equals to `expected`, otherwise set `expected` to the current value.
	template<MemoryOrder successMemoryOrder = MemoryOrder::SequentiallyConsistent, MemoryOrder failureMemoryOrder = MemoryOrder::SequentiallyConsistent>
	constexpr bool CompareGetSet(T& expected, T desired) noexcept {return AtomicCompareExchange<successMemoryOrder, failureMemoryOrder, false>(&mValue, &expected, desired);}

	/// Atomically add the argument to the previous value.
	/// @return the previous value.
	template<MemoryOrder mo = MemoryOrder::SequentiallyConsistent>
	constexpr T GetAdd(T rhs) noexcept requires CNumber<T> {return AtomicFetchAdd<mo>(&mValue, rhs);}
	template<MemoryOrder mo = MemoryOrder::SequentiallyConsistent>
	constexpr T GetAdd(index_t rhs) noexcept requires CBasicPointer<T> && (!CVoid<TRemovePointer<T>>) {return AtomicFetchAdd<mo>(&mValue, rhs*sizeof(*mValue));}

	/// Atomically subtract the argument from the previous value.
	/// @return the previous value.
	template<MemoryOrder mo = MemoryOrder::SequentiallyConsistent>
	constexpr T GetSub(T rhs) noexcept requires CNumber<T> {return AtomicFetchSub<mo>(&mValue, rhs);}
	template<MemoryOrder mo = MemoryOrder::SequentiallyConsistent>
	constexpr T GetSub(index_t rhs) noexcept requires CBasicPointer<T> && (!CVoid<TRemovePointer<T>>) {return AtomicFetchSub<mo>(&mValue, rhs*sizeof(*mValue));}

	/// Atomically perform bitwise AND between the argument and the previous value.
	/// @return the previous value.
	template<MemoryOrder mo = MemoryOrder::SequentiallyConsistent>
	constexpr T GetAnd(T rhs) noexcept requires CBasicIntegral<T> {return AtomicFetchAnd<mo>(&mValue, rhs);}

	/// Atomically perform bitwise OR between the argument and the previous value.
	/// @return the previous value.
	template<MemoryOrder mo = MemoryOrder::SequentiallyConsistent>
	constexpr T GetOr(T rhs) noexcept requires CBasicIntegral<T> {return AtomicFetchOr<mo>(&mValue, rhs);}

	/// Atomically perform bitwise XOR between the argument and the previous value.
	/// @return the previous value.
	template<MemoryOrder mo = MemoryOrder::SequentiallyConsistent>
	constexpr T GetXor(T rhs) noexcept requires CBasicIntegral<T> {return AtomicFetchXor<mo>(&mValue, rhs);}

	/// Atomically add the argument to the previous value.
	/// @return the resulting value.
	template<MemoryOrder mo = MemoryOrder::SequentiallyConsistent>
	constexpr T Add(T rhs) noexcept requires CNumber<T> {return AtomicAddFetch<mo>(&mValue, rhs);}
	template<MemoryOrder mo = MemoryOrder::SequentiallyConsistent>
	constexpr T Add(index_t rhs) noexcept requires CBasicPointer<T> && (!CVoid<TRemovePointer<T>>) {return AtomicAddFetch<mo>(&mValue, rhs*sizeof(*mValue));}

	/// Atomically subtract the argument from the previous value.
	/// @return the resulting value.
	template<MemoryOrder mo = MemoryOrder::SequentiallyConsistent>
	constexpr T Sub(T rhs) noexcept requires CNumber<T> {return AtomicSubFetch<mo>(&mValue, rhs);}
	template<MemoryOrder mo = MemoryOrder::SequentiallyConsistent>
	constexpr T Sub(index_t rhs) noexcept requires CBasicPointer<T> && (!CVoid<TRemovePointer<T>>) {return AtomicSubFetch<mo>(&mValue, rhs*sizeof(*mValue));}

	/// Atomically perform bitwise AND between the argument and the previous value.
	/// @return the resulting value.
	template<MemoryOrder mo = MemoryOrder::SequentiallyConsistent>
	constexpr T And(T rhs) noexcept requires CBasicIntegral<T> {return AtomicAndFetch<mo>(&mValue, rhs);}

	/// Atomically perform bitwise OR between the argument and the previous value.
	/// @return the resulting value.
	template<MemoryOrder mo = MemoryOrder::SequentiallyConsistent>
	constexpr T Or(T rhs) noexcept requires CBasicIntegral<T> {return AtomicOrFetch<mo>(&mValue, rhs);}

	/// Atomically perform bitwise XOR between the argument and the previous value.
	/// @return the resulting value.
	template<MemoryOrder mo = MemoryOrder::SequentiallyConsistent>
	constexpr T Xor(T rhs) noexcept requires CBasicIntegral<T> {return AtomicXorFetch<mo>(&mValue, rhs);}

private:
	T mValue;
};

/// Implements non-atomic equivalents to the operations of Atomic<T>.
/// Useful in templated code.
template<typename T> class FakeAtomic
{
	FakeAtomic(const FakeAtomic&) = delete;
	FakeAtomic& operator=(const FakeAtomic&) = delete;
public:
	constexpr FakeAtomic() noexcept: mValue{} {}
	constexpr FakeAtomic(T val) noexcept: mValue{val} {}

	/// @return Current value.
	template<MemoryOrder mo = MemoryOrder::SequentiallyConsistent>
	constexpr T Get() const noexcept {return mValue;}

	/// Set a new value.
	/// @return The previous value.
	template<MemoryOrder mo = MemoryOrder::SequentiallyConsistent>
	constexpr T GetSet(T val) noexcept {return Exchange(mValue, val);}

	/// Change the flag state to true.
	/// @return The previous value of the flag.
	template<MemoryOrder mo = MemoryOrder::SequentiallyConsistent>
	constexpr bool GetSet() noexcept requires CSame<T, bool> {auto res = mValue; mValue = true; return res;}

	/// Set a new value.
	template<MemoryOrder mo = MemoryOrder::SequentiallyConsistent>
	constexpr void Set(T val) noexcept {mValue = val;}

	/// Reset the flag.
	template<MemoryOrder mo = MemoryOrder::SequentiallyConsistent>
	constexpr void Reset() noexcept requires CSame<T, bool> {mValue = false;}

	constexpr operator bool() requires CSame<T, bool> {return Get();}

	constexpr bool IsAlwaysLockFree = true;

	/// Set value desired if the current value equals to expected.
	template<MemoryOrder successMemoryOrder = MemoryOrder::SequentiallyConsistent, MemoryOrder failureMemoryOrder = MemoryOrder::SequentiallyConsistent>
	constexpr bool WeakCompareSet(T expected, T desired) noexcept {return CompareExchange(&mValue, &expected, desired);}

	/// Set value desired if the current value equals to expected, otherwise set expected to the current value.
	template<MemoryOrder successMemoryOrder = MemoryOrder::SequentiallyConsistent, MemoryOrder failureMemoryOrder = MemoryOrder::SequentiallyConsistent>
	constexpr bool WeakCompareGetSet(T& expected, T desired) noexcept {return CompareExchange(&mValue, &expected, desired);}

	/// Set value desired if the current value equals to expected.
	template<MemoryOrder successMemoryOrder = MemoryOrder::SequentiallyConsistent, MemoryOrder failureMemoryOrder = MemoryOrder::SequentiallyConsistent>
	constexpr bool CompareSet(T expected, T desired) noexcept {return CompareExchange(&mValue, &expected, desired);}

	/// Set value `desired` if the current value equals to `expected`, otherwise set `expected` to the current value.
	template<MemoryOrder successMemoryOrder = MemoryOrder::SequentiallyConsistent, MemoryOrder failureMemoryOrder = MemoryOrder::SequentiallyConsistent>
	constexpr bool CompareGetSet(T& expected, T desired) noexcept {return CompareExchange(&mValue, &expected, desired);}

	/// Add the argument to the previous value.
	/// @return The previous value.
	template<MemoryOrder mo = MemoryOrder::SequentiallyConsistent>
	constexpr T GetAdd(T rhs) noexcept requires CNumber<T> {return (mValue += rhs) - rhs;}
	template<MemoryOrder mo = MemoryOrder::SequentiallyConsistent>
	constexpr T GetAdd(index_t rhs) noexcept requires CBasicPointer<T> && (!CVoid<TRemovePointer<T>>) {return (mValue += rhs) - rhs;}

	/// Subtract the argument from the previous value.
	/// @return The previous value.
	template<MemoryOrder mo = MemoryOrder::SequentiallyConsistent>
	constexpr T GetSub(T rhs) noexcept requires CNumber<T> {return AtomicFetchSub<mo>(&mValue, rhs);}
	template<MemoryOrder mo = MemoryOrder::SequentiallyConsistent>
	constexpr T GetSub(index_t rhs) noexcept requires CBasicPointer<T> && (!CVoid<TRemovePointer<T>>) {return (mValue -= rhs) + rhs;}

	/// Perform bitwise AND between the argument and the previous value.
	/// @return The previous value.
	template<MemoryOrder mo = MemoryOrder::SequentiallyConsistent>
	constexpr T GetAnd(T rhs) noexcept requires CBasicIntegral<T> {auto res = mValue; res &= rhs; return res;}

	/// Perform bitwise OR between the argument and the previous value.
	/// @return The previous value.
	template<MemoryOrder mo = MemoryOrder::SequentiallyConsistent>
	constexpr T GetOr(T rhs) noexcept requires CBasicIntegral<T> {auto res = mValue; res |= rhs; return res;}

	/// Perform bitwise XOR between the argument and the previous value.
	/// @return The previous value.
	template<MemoryOrder mo = MemoryOrder::SequentiallyConsistent>
	constexpr T GetXor(T rhs) noexcept requires CBasicIntegral<T> {auto res = mValue; res ^= rhs; return res;}

	/// Add the argument to the previous value.
	/// @return The resulting value.
	template<MemoryOrder mo = MemoryOrder::SequentiallyConsistent>
	constexpr T Add(T rhs) noexcept requires CNumber<T> {return mValue += rhs;}
	template<MemoryOrder mo = MemoryOrder::SequentiallyConsistent>
	constexpr T Add(index_t rhs) noexcept requires CBasicPointer<T> && (!CVoid<TRemovePointer<T>>) {return mValue += rhs;}

	/// Subtract the argument from the previous value.
	/// @return The resulting value.
	template<MemoryOrder mo = MemoryOrder::SequentiallyConsistent>
	constexpr T Sub(T rhs) noexcept requires CNumber<T> {return mValue -= rhs;}
	template<MemoryOrder mo = MemoryOrder::SequentiallyConsistent>
	constexpr T Sub(index_t rhs) noexcept requires CBasicPointer<T> && (!CVoid<TRemovePointer<T>>) {return mValue -= rhs;}

	/// Perform bitwise AND between the argument and the previous value.
	/// @return the resulting value.
	template<MemoryOrder mo = MemoryOrder::SequentiallyConsistent>
	constexpr T And(T rhs) noexcept requires CBasicIntegral<T> {return mValue &= rhs;}

	/// Perform bitwise OR between the argument and the previous value.
	/// @return the resulting value.
	template<MemoryOrder mo = MemoryOrder::SequentiallyConsistent>
	constexpr T Or(T rhs) noexcept requires CBasicIntegral<T> {return mValue |= rhs;}

	/// Perform bitwise XOR between the argument and the previous value.
	/// @return the resulting value.
	template<MemoryOrder mo = MemoryOrder::SequentiallyConsistent>
	constexpr T Xor(T rhs) noexcept requires CBasicIntegral<T> {return mValue ^= rhs;}

private:
	T mValue;
};

template<typename T, bool ThreadSafe> using TSelectAtomic = TSelect<Atomic<T>, FakeAtomic<T>, ThreadSafe>;

} INTRA_END
