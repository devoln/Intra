#pragma once

#include "Cpp/PlatformDetect.h"
#include "Cpp/Features.h"
#include "Cpp/Warnings.h"

#include "Thread.h"

#define INTRA_LIBRARY_ATOMIC_None 0
#define INTRA_LIBRARY_ATOMIC_MSVC 1
#define INTRA_LIBRARY_ATOMIC_GNU 2
#define INTRA_LIBRARY_ATOMIC_Cpp11 3

//#define INTRA_LIBRARY_ATOMIC INTRA_LIBRARY_ATOMIC_GNU

#ifndef INTRA_LIBRARY_ATOMIC

#if(defined(INTRA_LIBRARY_THREAD) && INTRA_LIBRARY_THREAD == INTRA_LIBRARY_THREAD_Cpp11)
#define INTRA_LIBRARY_ATOMIC INTRA_LIBRARY_ATOMIC_Cpp11
#elif defined(_MSC_VER) && (!defined(__clang__) || defined(__c2__))
#define INTRA_LIBRARY_ATOMIC INTRA_LIBRARY_ATOMIC_MSVC
#elif(INTRA_PLATFORM_OS == INTRA_PLATFORM_OS_Emscripten && !defined(__EMSCRIPTEN_PTHREADS__))
#define INTRA_LIBRARY_ATOMIC INTRA_LIBRARY_ATOMIC_None
#else
#define INTRA_LIBRARY_ATOMIC INTRA_LIBRARY_ATOMIC_GNU
#endif

#endif

INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS

#if(INTRA_LIBRARY_ATOMIC == INTRA_LIBRARY_ATOMIC_Cpp11)
#include <atomic>
#endif

namespace Intra { namespace Concurrency {

//! Это перечисление описывает, как должен быть упорядочен обычный (неатомарный) доступ к памяти по отношению к атомарной операции.
enum class MemoryOrder
{
	//! Никаких ограничений на порядок к доступу памяти не накладывается.
	//! Гарантируется только атомарность операции.
	//! Чаще всего применяется, когда нужна атомарность самой переменной,
	//! и эта переменная не используется для синхронизации других общих данных.
	Relaxed,

	Consume,

	Acquire,

	Release,

	//! Комбинация Acquire и Release.
	AcquireRelease,

	//! Все ограничения AcquireRelease, а также
	//! существует единый общий порядок, при котором все потоки видят все изменения.
	//! Является наиболее интуитивно понятным, но препятствует оптимизации.
	SequentiallyConsistent
};

template<typename T> class AtomicBase
{
	AtomicBase(const AtomicBase&) = delete;
	AtomicBase& operator=(const AtomicBase&) = delete;
public:
	AtomicBase() noexcept: mValue{} {}
	AtomicBase(T val) noexcept: mValue{val} {}

	//! @defgroup AtomicBase_Get
	//! Возвращает текущее значение.
	//!@{
	forceinline T Get() const noexcept;
	forceinline T GetRelaxed() const noexcept;
	forceinline T GetConsume() const noexcept;
	forceinline T GetAcquire() const noexcept;
	//!@}


	
	//! @defgroup AtomicBase_GetSet
	//! Атомарно устанавливает новое значение и возвращает предыдущее.
	//!@{
	forceinline T GetSet(T val) noexcept;
	forceinline T GetSetRelaxed(T val) noexcept;
	forceinline T GetSetConsume(T val) noexcept;
	forceinline T GetSetAcquire(T val) noexcept;
	forceinline T GetSetRelease(T val) noexcept;
	forceinline T GetSetAcquireRelease(T val) noexcept;
	//!@}

	
	//! @defgroup AtomicBase_Set
	//! Устанавливает новое значение.
	//!@{
	forceinline void Set(T val) noexcept;
	forceinline void SetRelaxed(T val) noexcept;
	forceinline void SetRelease(T val) noexcept;
	//!@}


	//! @defgroup AtomicBase_WeakCompareSet
	//! Атомарно устанавливает значение desired, если текущее значение равно expected (*), иначе записывает в expected текущее значение.
	//! * С некоторой вероятностью может не сработать, поэтому нужно вызывать её в цикле или использовать CompareSet.
	//!@{
	forceinline bool WeakCompareSet(T expected, T desired) noexcept;
	forceinline bool WeakCompareSetRelaxed(T expected, T desired) noexcept;
	forceinline bool WeakCompareSetConsume(T expected, T desired) noexcept;
	forceinline bool WeakCompareSetAcquire(T expected, T desired) noexcept;
	forceinline bool WeakCompareSetRelease(T expected, T desired) noexcept;
	forceinline bool WeakCompareSetAcquireRelease(T expected, T desired) noexcept;
	//!@}

	//! @defgroup AtomicBase_WeakCompareSet
	//! Атомарно устанавливает значение desired, если текущее значение равно expected (*), иначе записывает в expected текущее значение.
	//! * С некоторой вероятностью может не сработать, поэтому нужно вызывать её в цикле или использовать CompareGetSet.
	//!@{
	forceinline bool WeakCompareGetSet(T& expected, T desired) noexcept;
	forceinline bool WeakCompareGetSetRelaxed(T& expected, T desired) noexcept;
	forceinline bool WeakCompareGetSetConsume(T& expected, T desired) noexcept;
	forceinline bool WeakCompareGetSetAcquire(T& expected, T desired) noexcept;
	forceinline bool WeakCompareGetSetRelease(T& expected, T desired) noexcept;
	forceinline bool WeakCompareGetSetAcquireRelease(T& expected, T desired) noexcept;
	//!@}

	//! @defgroup AtomicBase_CompareSet
	//! Атомарно устанавливает значение desired, если текущее значение равно expected, иначе записывает в expected текущее значение.
	//! На некоторых платформах может быть медленнее, чем WeakCompareSet.
	//!@{
	forceinline bool CompareSet(T expected, T desired) noexcept;
	forceinline bool CompareSetRelaxed(T expected, T desired) noexcept;
	forceinline bool CompareSetConsume(T expected, T desired) noexcept;
	forceinline bool CompareSetAcquire(T expected, T desired) noexcept;
	forceinline bool CompareSetRelease(T expected, T desired) noexcept;
	forceinline bool CompareSetAcquireRelease(T expected, T desired) noexcept;
	//!@}

	//! @defgroup AtomicBase_CompareSet
	//! Атомарно устанавливает значение desired, если текущее значение равно expected.
	//! На некоторых платформах может быть медленнее, чем WeakCompareGetSet.
	//!@{
	forceinline bool CompareGetSet(T& expected, T desired) noexcept;
	forceinline bool CompareGetSetRelaxed(T& expected, T desired) noexcept;
	forceinline bool CompareGetSetConsume(T& expected, T desired) noexcept;
	forceinline bool CompareGetSetAcquire(T& expected, T desired) noexcept;
	forceinline bool CompareGetSetRelease(T& expected, T desired) noexcept;
	forceinline bool CompareGetSetAcquireRelease(T& expected, T desired) noexcept;
	//!@}

protected:
#if(INTRA_LIBRARY_ATOMIC == INTRA_LIBRARY_ATOMIC_Cpp11)
	std::atomic<T> mValue;
#else
	T mValue;
#endif
};

template<typename T> class AtomicInteger: public AtomicBase<T>
{
	AtomicInteger(const AtomicInteger&) = delete;
	AtomicInteger& operator=(const AtomicInteger&) = delete;
public:
	AtomicInteger(T value = T()): AtomicBase<T>(value) {};

	//! @defgroup AtomicInteger_GetAdd
	//! Прибавляет число к теущему значению и возвращает предыдущее значение.
	//!@{
	forceinline T GetAdd(T rhs) noexcept;
	forceinline T GetAddRelaxed(T rhs) noexcept;
	forceinline T GetAddConsume(T rhs) noexcept;
	forceinline T GetAddAcquire(T rhs) noexcept;
	forceinline T GetAddRelease(T rhs) noexcept;
	forceinline T GetAddAcquireRelease(T rhs) noexcept;
	//!@}

	//! @defgroup AtomicInteger_GetSub
	//! Вычитает число из теущего значения и возвращает предыдущее значение.
	//!@{
	forceinline T GetSub(T rhs) noexcept;
	forceinline T GetSubRelaxed(T rhs) noexcept;
	forceinline T GetSubConsume(T rhs) noexcept;
	forceinline T GetSubAcquire(T rhs) noexcept;
	forceinline T GetSubRelease(T rhs) noexcept;
	forceinline T GetSubAcquireRelease(T rhs) noexcept;
	//!@}

	//! @defgroup AtomicInteger_GetAnd
	//! Применяет побитовое И к теущему значению и возвращает предыдущее значение.
	//!@{
	forceinline T GetAnd(T rhs) noexcept;
	forceinline T GetAndRelaxed(T rhs) noexcept;
	forceinline T GetAndConsume(T rhs) noexcept;
	forceinline T GetAndAcquire(T rhs) noexcept;
	forceinline T GetAndRelease(T rhs) noexcept;
	forceinline T GetAndAcquireRelease(T rhs) noexcept;
	//!@}

	//! @defgroup AtomicInteger_GetOr
	//! Применяет побитовое ИЛИ к теущему значению и возвращает предыдущее значение.
	//!@{
	forceinline T GetOr(T rhs) noexcept;
	forceinline T GetOrRelaxed(T rhs) noexcept;
	forceinline T GetOrConsume(T rhs) noexcept;
	forceinline T GetOrAcquire(T rhs) noexcept;
	forceinline T GetOrRelease(T rhs) noexcept;
	forceinline T GetOrAcquireRelease(T rhs) noexcept;
	//!@}

	//! @defgroup AtomicInteger_GetXor
	//! Применяет побитовое ИСКЛ. ИЛИ к теущему значению и возвращает предыдущее значение.
	//!@{
	forceinline T GetXor(T rhs) noexcept;
	forceinline T GetXorRelaxed(T rhs) noexcept;
	forceinline T GetXorConsume(T rhs) noexcept;
	forceinline T GetXorAcquire(T rhs) noexcept;
	forceinline T GetXorRelease(T rhs) noexcept;
	forceinline T GetXorAcquireRelease(T rhs) noexcept;
	//!@}


	//! @defgroup AtomicInteger_Add
	//! Прибавляет число к теущему значению и возвращает новое значение.
	//!@{
	forceinline T Add(T rhs) noexcept;
	forceinline T AddRelaxed(T rhs) noexcept;
	forceinline T AddConsume(T rhs) noexcept;
	forceinline T AddAcquire(T rhs) noexcept;
	forceinline T AddRelease(T rhs) noexcept;
	forceinline T AddAcquireRelease(T rhs) noexcept;
	//!@}

	//! @defgroup AtomicInteger_Sub
	//! Вычитает число из теущего значения и возвращает новое значение.
	//!@{
	forceinline T Sub(T rhs) noexcept;
	forceinline T SubRelaxed(T rhs) noexcept;
	forceinline T SubConsume(T rhs) noexcept;
	forceinline T SubAcquire(T rhs) noexcept;
	forceinline T SubRelease(T rhs) noexcept;
	forceinline T SubAcquireRelease(T rhs) noexcept;
	//!@}

	//! @defgroup AtomicInteger_And
	//! Применяет побитовое И к теущему значению и возвращает новое значение.
	//!@{
	forceinline T And(T rhs) noexcept;
	forceinline T AndRelaxed(T rhs) noexcept;
	forceinline T AndConsume(T rhs) noexcept;
	forceinline T AndAcquire(T rhs) noexcept;
	forceinline T AndRelease(T rhs) noexcept;
	forceinline T AndAcquireRelease(T rhs) noexcept;
	//!@}

	//! @defgroup AtomicInteger_Or
	//! Применяет побитовое ИЛИ к теущему значению и возвращает новое значение.
	//!@{
	forceinline T Or(T rhs) noexcept;
	forceinline T OrRelaxed(T rhs) noexcept;
	forceinline T OrConsume(T rhs) noexcept;
	forceinline T OrAcquire(T rhs) noexcept;
	forceinline T OrRelease(T rhs) noexcept;
	forceinline T OrAcquireRelease(T rhs) noexcept;
	//!@}

	//! @defgroup AtomicInteger_Xor
	//! Применяет побитовое ИСКЛ. ИЛИ к теущему значению и возвращает новое значение.
	//!@{
	forceinline T Xor(T rhs) noexcept;
	forceinline T XorRelaxed(T rhs) noexcept;
	forceinline T XorConsume(T rhs) noexcept;
	forceinline T XorAcquire(T rhs) noexcept;
	forceinline T XorRelease(T rhs) noexcept;
	forceinline T XorAcquireRelease(T rhs) noexcept;
	//!@}


	//! @defgroup AtomicInteger_Increment
	//! Увеличивает текущее значение на 1 и возвращает новое значение.
	//!@{
	forceinline T Increment() noexcept;
	forceinline T IncrementRelaxed() noexcept;
	forceinline T IncrementConsume() noexcept;
	forceinline T IncrementAcquire() noexcept;
	forceinline T IncrementRelease() noexcept;
	forceinline T IncrementAcquireRelease() noexcept;
	//!@}

	//! @defgroup AtomicInteger_Decrement
	//! Уменьшает текущее значение на 1 и возвращает новое значение.
	//!@{
	forceinline T Decrement() noexcept;
	forceinline T DecrementRelaxed() noexcept;
	forceinline T DecrementConsume() noexcept;
	forceinline T DecrementAcquire() noexcept;
	forceinline T DecrementRelease() noexcept;
	forceinline T DecrementAcquireRelease() noexcept;
	//!@}

	//! @defgroup AtomicInteger_GetIncrement
	//! Увеличивает текущее значение на 1 и возвращает старое значение.
	//!@{
	forceinline T GetIncrement() noexcept;
	forceinline T GetIncrementRelaxed() noexcept;
	forceinline T GetIncrementConsume() noexcept;
	forceinline T GetIncrementAcquire() noexcept;
	forceinline T GetIncrementRelease() noexcept;
	forceinline T GetIncrementAcquireRelease() noexcept;
	//!@}

	//! @defgroup AtomicInteger_GetDecrement
	//! Уменьшает текущее значение на 1 и возвращает старое значение.
	//!@{
	forceinline T GetDecrement() noexcept;
	forceinline T GetDecrementRelaxed() noexcept;
	forceinline T GetDecrementConsume() noexcept;
	forceinline T GetDecrementAcquire() noexcept;
	forceinline T GetDecrementRelease() noexcept;
	forceinline T GetDecrementAcquireRelease() noexcept;
	//!@}
};

typedef AtomicBase<bool> AtomicBool;
typedef AtomicInteger<int> AtomicInt;
typedef AtomicInteger<long64> AtomicLong;

}}

#if(INTRA_LIBRARY_ATOMIC == INTRA_LIBRARY_ATOMIC_Cpp11)
#include "detail/AtomicCpp11.h"
#elif(INTRA_LIBRARY_ATOMIC == INTRA_LIBRARY_ATOMIC_MSVC)
#include "detail/AtomicMSVC.h"
#elif(INTRA_LIBRARY_ATOMIC == INTRA_LIBRARY_ATOMIC_GNU)
#include "detail/AtomicGNU.h"
#endif

namespace Intra {
using Concurrency::AtomicBool;
using Concurrency::AtomicInt;
using Concurrency::AtomicLong;
}

INTRA_WARNING_POP
