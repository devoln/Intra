#pragma once

#include "Intra/Core.h"

#define INTRA_LIBRARY_ATOMIC_None 0
#define INTRA_LIBRARY_ATOMIC_MSVC 1
#define INTRA_LIBRARY_ATOMIC_GNU 2
#define INTRA_LIBRARY_ATOMIC_Cpp11 3

//#define INTRA_LIBRARY_ATOMIC INTRA_LIBRARY_ATOMIC_GNU

#ifndef INTRA_LIBRARY_ATOMIC

#if defined(_MSC_VER) && (!defined(__clang__) || defined(__c2__))
#define INTRA_LIBRARY_ATOMIC INTRA_LIBRARY_ATOMIC_MSVC
#elif defined(__Emscripten__) && !defined(__EMSCRIPTEN_PTHREADS__)
#define INTRA_LIBRARY_ATOMIC INTRA_LIBRARY_ATOMIC_None
#else
#define INTRA_LIBRARY_ATOMIC INTRA_LIBRARY_ATOMIC_GNU
#endif

#endif

#if(INTRA_LIBRARY_ATOMIC != INTRA_LIBRARY_ATOMIC_None)

INTRA_BEGIN
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
	INTRA_FORCEINLINE T Get() const noexcept;
	INTRA_FORCEINLINE T GetRelaxed() const noexcept;
	INTRA_FORCEINLINE T GetConsume() const noexcept;
	INTRA_FORCEINLINE T GetAcquire() const noexcept;
	//!@}


	
	//! @defgroup AtomicBase_GetSet
	//! Атомарно устанавливает новое значение и возвращает предыдущее.
	//!@{
	INTRA_FORCEINLINE T GetSet(T val) noexcept;
	INTRA_FORCEINLINE T GetSetRelaxed(T val) noexcept;
	INTRA_FORCEINLINE T GetSetConsume(T val) noexcept;
	INTRA_FORCEINLINE T GetSetAcquire(T val) noexcept;
	INTRA_FORCEINLINE T GetSetRelease(T val) noexcept;
	INTRA_FORCEINLINE T GetSetAcquireRelease(T val) noexcept;
	//!@}

	
	//! @defgroup AtomicBase_Set
	//! Устанавливает новое значение.
	//!@{
	INTRA_FORCEINLINE void Set(T val) noexcept;
	INTRA_FORCEINLINE void SetRelaxed(T val) noexcept;
	INTRA_FORCEINLINE void SetRelease(T val) noexcept;
	//!@}


	//! @defgroup AtomicBase_WeakCompareSet
	//! Атомарно устанавливает значение desired, если текущее значение равно expected (*), иначе записывает в expected текущее значение.
	//! * С некоторой вероятностью может не сработать, поэтому нужно вызывать её в цикле или использовать CompareSet.
	//!@{
	INTRA_FORCEINLINE bool WeakCompareSet(T expected, T desired) noexcept;
	INTRA_FORCEINLINE bool WeakCompareSetRelaxed(T expected, T desired) noexcept;
	INTRA_FORCEINLINE bool WeakCompareSetConsume(T expected, T desired) noexcept;
	INTRA_FORCEINLINE bool WeakCompareSetAcquire(T expected, T desired) noexcept;
	INTRA_FORCEINLINE bool WeakCompareSetRelease(T expected, T desired) noexcept;
	INTRA_FORCEINLINE bool WeakCompareSetAcquireRelease(T expected, T desired) noexcept;
	//!@}

	//! @defgroup AtomicBase_WeakCompareSet
	//! Атомарно устанавливает значение desired, если текущее значение равно expected (*), иначе записывает в expected текущее значение.
	//! * С некоторой вероятностью может не сработать, поэтому нужно вызывать её в цикле или использовать CompareGetSet.
	//!@{
	INTRA_FORCEINLINE bool WeakCompareGetSet(T& expected, T desired) noexcept;
	INTRA_FORCEINLINE bool WeakCompareGetSetRelaxed(T& expected, T desired) noexcept;
	INTRA_FORCEINLINE bool WeakCompareGetSetConsume(T& expected, T desired) noexcept;
	INTRA_FORCEINLINE bool WeakCompareGetSetAcquire(T& expected, T desired) noexcept;
	INTRA_FORCEINLINE bool WeakCompareGetSetRelease(T& expected, T desired) noexcept;
	INTRA_FORCEINLINE bool WeakCompareGetSetAcquireRelease(T& expected, T desired) noexcept;
	//!@}

	//! @defgroup AtomicBase_CompareSet
	//! Атомарно устанавливает значение desired, если текущее значение равно expected, иначе записывает в expected текущее значение.
	//! На некоторых платформах может быть медленнее, чем WeakCompareSet.
	//!@{
	INTRA_FORCEINLINE bool CompareSet(T expected, T desired) noexcept;
	INTRA_FORCEINLINE bool CompareSetRelaxed(T expected, T desired) noexcept;
	INTRA_FORCEINLINE bool CompareSetConsume(T expected, T desired) noexcept;
	INTRA_FORCEINLINE bool CompareSetAcquire(T expected, T desired) noexcept;
	INTRA_FORCEINLINE bool CompareSetRelease(T expected, T desired) noexcept;
	INTRA_FORCEINLINE bool CompareSetAcquireRelease(T expected, T desired) noexcept;
	//!@}

	//! @defgroup AtomicBase_CompareSet
	//! Атомарно устанавливает значение desired, если текущее значение равно expected.
	//! На некоторых платформах может быть медленнее, чем WeakCompareGetSet.
	//!@{
	INTRA_FORCEINLINE bool CompareGetSet(T& expected, T desired) noexcept;
	INTRA_FORCEINLINE bool CompareGetSetRelaxed(T& expected, T desired) noexcept;
	INTRA_FORCEINLINE bool CompareGetSetConsume(T& expected, T desired) noexcept;
	INTRA_FORCEINLINE bool CompareGetSetAcquire(T& expected, T desired) noexcept;
	INTRA_FORCEINLINE bool CompareGetSetRelease(T& expected, T desired) noexcept;
	INTRA_FORCEINLINE bool CompareGetSetAcquireRelease(T& expected, T desired) noexcept;
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
	AtomicInteger(T value = T()): AtomicBase<T>(value) {}

	//! @defgroup AtomicInteger_GetAdd
	//! Прибавляет число к теущему значению и возвращает предыдущее значение.
	//!@{
	INTRA_FORCEINLINE T GetAdd(T rhs) noexcept;
	INTRA_FORCEINLINE T GetAddRelaxed(T rhs) noexcept;
	INTRA_FORCEINLINE T GetAddConsume(T rhs) noexcept;
	INTRA_FORCEINLINE T GetAddAcquire(T rhs) noexcept;
	INTRA_FORCEINLINE T GetAddRelease(T rhs) noexcept;
	INTRA_FORCEINLINE T GetAddAcquireRelease(T rhs) noexcept;
	//!@}

	//! @defgroup AtomicInteger_GetSub
	//! Вычитает число из теущего значения и возвращает предыдущее значение.
	//!@{
	INTRA_FORCEINLINE T GetSub(T rhs) noexcept;
	INTRA_FORCEINLINE T GetSubRelaxed(T rhs) noexcept;
	INTRA_FORCEINLINE T GetSubConsume(T rhs) noexcept;
	INTRA_FORCEINLINE T GetSubAcquire(T rhs) noexcept;
	INTRA_FORCEINLINE T GetSubRelease(T rhs) noexcept;
	INTRA_FORCEINLINE T GetSubAcquireRelease(T rhs) noexcept;
	//!@}

	//! @defgroup AtomicInteger_GetAnd
	//! Применяет побитовое И к теущему значению и возвращает предыдущее значение.
	//!@{
	INTRA_FORCEINLINE T GetAnd(T rhs) noexcept;
	INTRA_FORCEINLINE T GetAndRelaxed(T rhs) noexcept;
	INTRA_FORCEINLINE T GetAndConsume(T rhs) noexcept;
	INTRA_FORCEINLINE T GetAndAcquire(T rhs) noexcept;
	INTRA_FORCEINLINE T GetAndRelease(T rhs) noexcept;
	INTRA_FORCEINLINE T GetAndAcquireRelease(T rhs) noexcept;
	//!@}

	//! @defgroup AtomicInteger_GetOr
	//! Применяет побитовое ИЛИ к теущему значению и возвращает предыдущее значение.
	//!@{
	INTRA_FORCEINLINE T GetOr(T rhs) noexcept;
	INTRA_FORCEINLINE T GetOrRelaxed(T rhs) noexcept;
	INTRA_FORCEINLINE T GetOrConsume(T rhs) noexcept;
	INTRA_FORCEINLINE T GetOrAcquire(T rhs) noexcept;
	INTRA_FORCEINLINE T GetOrRelease(T rhs) noexcept;
	INTRA_FORCEINLINE T GetOrAcquireRelease(T rhs) noexcept;
	//!@}

	//! @defgroup AtomicInteger_GetXor
	//! Применяет побитовое ИСКЛ. ИЛИ к теущему значению и возвращает предыдущее значение.
	//!@{
	INTRA_FORCEINLINE T GetXor(T rhs) noexcept;
	INTRA_FORCEINLINE T GetXorRelaxed(T rhs) noexcept;
	INTRA_FORCEINLINE T GetXorConsume(T rhs) noexcept;
	INTRA_FORCEINLINE T GetXorAcquire(T rhs) noexcept;
	INTRA_FORCEINLINE T GetXorRelease(T rhs) noexcept;
	INTRA_FORCEINLINE T GetXorAcquireRelease(T rhs) noexcept;
	//!@}


	//! @defgroup AtomicInteger_Add
	//! Прибавляет число к теущему значению и возвращает новое значение.
	//!@{
	INTRA_FORCEINLINE T Add(T rhs) noexcept;
	INTRA_FORCEINLINE T AddRelaxed(T rhs) noexcept;
	INTRA_FORCEINLINE T AddConsume(T rhs) noexcept;
	INTRA_FORCEINLINE T AddAcquire(T rhs) noexcept;
	INTRA_FORCEINLINE T AddRelease(T rhs) noexcept;
	INTRA_FORCEINLINE T AddAcquireRelease(T rhs) noexcept;
	//!@}

	//! @defgroup AtomicInteger_Sub
	//! Вычитает число из теущего значения и возвращает новое значение.
	//!@{
	INTRA_FORCEINLINE T Sub(T rhs) noexcept;
	INTRA_FORCEINLINE T SubRelaxed(T rhs) noexcept;
	INTRA_FORCEINLINE T SubConsume(T rhs) noexcept;
	INTRA_FORCEINLINE T SubAcquire(T rhs) noexcept;
	INTRA_FORCEINLINE T SubRelease(T rhs) noexcept;
	INTRA_FORCEINLINE T SubAcquireRelease(T rhs) noexcept;
	//!@}

	//! @defgroup AtomicInteger_And
	//! Применяет побитовое И к теущему значению и возвращает новое значение.
	//!@{
	INTRA_FORCEINLINE T And(T rhs) noexcept;
	INTRA_FORCEINLINE T AndRelaxed(T rhs) noexcept;
	INTRA_FORCEINLINE T AndConsume(T rhs) noexcept;
	INTRA_FORCEINLINE T AndAcquire(T rhs) noexcept;
	INTRA_FORCEINLINE T AndRelease(T rhs) noexcept;
	INTRA_FORCEINLINE T AndAcquireRelease(T rhs) noexcept;
	//!@}

	//! @defgroup AtomicInteger_Or
	//! Применяет побитовое ИЛИ к теущему значению и возвращает новое значение.
	//!@{
	INTRA_FORCEINLINE T Or(T rhs) noexcept;
	INTRA_FORCEINLINE T OrRelaxed(T rhs) noexcept;
	INTRA_FORCEINLINE T OrConsume(T rhs) noexcept;
	INTRA_FORCEINLINE T OrAcquire(T rhs) noexcept;
	INTRA_FORCEINLINE T OrRelease(T rhs) noexcept;
	INTRA_FORCEINLINE T OrAcquireRelease(T rhs) noexcept;
	//!@}

	//! @defgroup AtomicInteger_Xor
	//! Применяет побитовое ИСКЛ. ИЛИ к теущему значению и возвращает новое значение.
	//!@{
	INTRA_FORCEINLINE T Xor(T rhs) noexcept;
	INTRA_FORCEINLINE T XorRelaxed(T rhs) noexcept;
	INTRA_FORCEINLINE T XorConsume(T rhs) noexcept;
	INTRA_FORCEINLINE T XorAcquire(T rhs) noexcept;
	INTRA_FORCEINLINE T XorRelease(T rhs) noexcept;
	INTRA_FORCEINLINE T XorAcquireRelease(T rhs) noexcept;
	//!@}


	//! @defgroup AtomicInteger_Increment
	//! Увеличивает текущее значение на 1 и возвращает новое значение.
	//!@{
	INTRA_FORCEINLINE T Increment() noexcept;
	INTRA_FORCEINLINE T IncrementRelaxed() noexcept;
	INTRA_FORCEINLINE T IncrementConsume() noexcept;
	INTRA_FORCEINLINE T IncrementAcquire() noexcept;
	INTRA_FORCEINLINE T IncrementRelease() noexcept;
	INTRA_FORCEINLINE T IncrementAcquireRelease() noexcept;
	//!@}

	//! @defgroup AtomicInteger_Decrement
	//! Уменьшает текущее значение на 1 и возвращает новое значение.
	//!@{
	INTRA_FORCEINLINE T Decrement() noexcept;
	INTRA_FORCEINLINE T DecrementRelaxed() noexcept;
	INTRA_FORCEINLINE T DecrementConsume() noexcept;
	INTRA_FORCEINLINE T DecrementAcquire() noexcept;
	INTRA_FORCEINLINE T DecrementRelease() noexcept;
	INTRA_FORCEINLINE T DecrementAcquireRelease() noexcept;
	//!@}

	//! @defgroup AtomicInteger_GetIncrement
	//! Увеличивает текущее значение на 1 и возвращает старое значение.
	//!@{
	INTRA_FORCEINLINE T GetIncrement() noexcept;
	INTRA_FORCEINLINE T GetIncrementRelaxed() noexcept;
	INTRA_FORCEINLINE T GetIncrementConsume() noexcept;
	INTRA_FORCEINLINE T GetIncrementAcquire() noexcept;
	INTRA_FORCEINLINE T GetIncrementRelease() noexcept;
	INTRA_FORCEINLINE T GetIncrementAcquireRelease() noexcept;
	//!@}

	//! @defgroup AtomicInteger_GetDecrement
	//! Уменьшает текущее значение на 1 и возвращает старое значение.
	//!@{
	INTRA_FORCEINLINE T GetDecrement() noexcept;
	INTRA_FORCEINLINE T GetDecrementRelaxed() noexcept;
	INTRA_FORCEINLINE T GetDecrementConsume() noexcept;
	INTRA_FORCEINLINE T GetDecrementAcquire() noexcept;
	INTRA_FORCEINLINE T GetDecrementRelease() noexcept;
	INTRA_FORCEINLINE T GetDecrementAcquireRelease() noexcept;
	//!@}
};

typedef AtomicBase<bool> AtomicBool;
typedef AtomicInteger<int> AtomicInt;
typedef AtomicInteger<int64> AtomicLong;

INTRA_END

#if(INTRA_LIBRARY_ATOMIC == INTRA_LIBRARY_ATOMIC_MSVC)
#include "detail/AtomicMSVC.h"
#elif(INTRA_LIBRARY_ATOMIC == INTRA_LIBRARY_ATOMIC_GNU)
#include "detail/AtomicGNU.h"
#endif

#endif
