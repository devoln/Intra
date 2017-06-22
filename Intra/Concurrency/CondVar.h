#pragma once

#include "Mutex.h"
#include "Lock.h"
#include "Thread.h"
#include "Synchronized.h"
#include "Container/Sequential/Array.h"

#include "System/DateTime.h"

INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS

namespace Intra { namespace Concurrency {

#if(INTRA_LIBRARY_MUTEX != INTRA_LIBRARY_MUTEX_None)
class SeparateCondVar
{
private:
public:
#if(INTRA_LIBRARY_MUTEX == INTRA_LIBRARY_MUTEX_Cpp11)
	enum {DATA_SIZE = 48}; //TODO: узнать точный размер std::condition_variable на разных платформах или хотя бы максимальный
#elif(INTRA_LIBRARY_MUTEX == INTRA_LIBRARY_MUTEX_WinAPI)
#ifdef INTRA_DROP_XP_SUPPORT
	enum {DATA_SIZE = sizeof(void*)};
#else
	enum {DATA_SIZE = sizeof(void*) + sizeof(Mutex) + sizeof(void*)*3*2 + sizeof(int)*2 + sizeof(int)};
#endif
#elif(INTRA_PLATFORM_OS == INTRA_PLATFORM_OS_Linux || INTRA_PLATFORM_OS == INTRA_PLATFORM_OS_Android)
	enum {DATA_SIZE = 48};
#elif(INTRA_PLATFORM_OS == INTRA_PLATFORM_OS_MacOS)
	enum {DATA_SIZE = 48}; //TODO: не проверено
#elif(INTRA_PLATFORM_OS == INTRA_PLATFORM_OS_iOS)
	enum {DATA_SIZE = 48}; //TODO: не проверено
#else
	enum {DATA_SIZE = 48}; //TODO: не проверено
#endif

	union
	{
		void* mForceAlignment;
		char mData[DATA_SIZE];
	};

#ifdef INTRA_DEBUG
	Synchronized<Array<Thread::Handle>> mWaiters;
#endif

public:
	SeparateCondVar();
	~SeparateCondVar();

	//! Указатель на реализацию.
	//! CONDITION_VARIABLE* для WinAPI или Cpp11 реализации на Windows Vista+
	//! null на Windows XP для WinAPI реализации
	//! pthread_cond_t* на остальных платформах.
	AnyPtr NativeHandle();

	//! Ждёт, пока не произойдёт одно из следующих событий:
	//! 1) NotifyAll или Notify, при котором будет выбран текущий поток, и при этом выполнится условие pred (вернёт true).
	//! 2) Прерывание текущего потока через Interrupt (вернёт false).
	//! До вызова функции мьютекс должен быть захвачен.
	//! Во время ожидания мьютекс свободен.
	//! После вызова мьютекс снова станет захвачен.
	//! @return Возвращает true, если условие pred в результате ожидания было выполнено.
	template<typename P> bool Wait(Mutex& mutex, P pred)
	{
		bool waitCondition;
#if !defined(INTRA_THREAD_NO_FULL_INTERRUPT) || defined(INTRA_DEBUG)
		ThisThread.onWait(this, &mutex);
#endif
#ifdef INTRA_DEBUG
		const auto thisThreadHandle = ThisThread.getHandle();
		if(thisThreadHandle) mWaiters->AddLast(thisThreadHandle);
#endif
		do waitCondition = pred();
		while(!waitCondition &&
#ifndef INTRA_THREAD_NO_FULL_INTERRUPT
			!ThisThread.IsInterrupted() &&
#endif
			wait(mutex)
		);
#if !defined(INTRA_THREAD_NO_FULL_INTERRUPT) || defined(INTRA_DEBUG)
		ThisThread.onWait(null, &mutex);
#endif
#ifdef INTRA_DEBUG
		if(thisThreadHandle) mWaiters->FindAndRemoveUnordered(thisThreadHandle);
#endif
		return waitCondition;
	}

	//! Ждёт, пока не произойдёт одно из следующих событий:
	//! 1) NotifyAll или Notify, при котором будет выбран текущий поток, и при этом выполнится условие pred (вернёт true).
	//! 2) Прерывание текущего потока через Interrupt (вернёт false).
	//! До вызова функции мьютекс должен быть захвачен.
	//! Во время ожидания мьютекс свободен.
	//! После вызова мьютекс снова станет захвачен.
	//! @return Возвращает true, если условие pred в результате ожидания было выполнено.
	template<typename L, typename P> bool Wait(Lock<L>& lock, P pred)
	{
		return Wait(lock.Primitive(), pred);
	}

	//! Ждёт, пока не произойдёт одно из следующих событий:
	//! 1) NotifyAll или Notify, при котором будет выбран текущий поток, и при этом выполнится условие pred (вернёт true).
	//! 2) Прерывание текущего потока через Interrupt (вернёт false).
	//! 3) Истечение таймаута (вернёт false).
	//! До вызова функции мьютекс должен быть захвачен.
	//! Во время ожидания мьютекс свободен.
	//! После вызова мьютекс снова станет захвачен.
	//! @return Возвращает true, если условие pred в результате ожидания было выполнено.
	template<typename P> bool WaitMs(Mutex& mutex, ulong64 timeout, P pred)
	{
		const ulong64 absTimeMs = System::DateTime::AbsTimeMs() + timeout;
		bool waitCondition;
#if !defined(INTRA_THREAD_NO_FULL_INTERRUPT) || defined(INTRA_DEBUG)
		ThisThread.onWait(this, &mutex);
#endif
#ifdef INTRA_DEBUG
		const auto thisThreadHandle = ThisThread.getHandle();
		mWaiters->AddLast(thisThreadHandle);
#endif
		do waitCondition = pred(); while(!waitCondition &&
#ifndef INTRA_THREAD_NO_FULL_INTERRUPT
			!ThisThread.IsInterrupted() &&
#endif
			waitUntil(mutex, absTimeMs));
#if !defined(INTRA_THREAD_NO_FULL_INTERRUPT) || defined(INTRA_DEBUG)
		ThisThread.onWait(null, &mutex);
#endif
#ifdef INTRA_DEBUG
		mWaiters->FindAndRemoveUnordered(thisThreadHandle);
#endif
		return waitCondition;
	}

	//! Ждёт, пока не произойдёт одно из следующих событий:
	//! 1) NotifyAll или Notify, при котором будет выбран текущий поток, и при этом выполнится условие pred (вернёт true).
	//! 2) Прерывание текущего потока через Interrupt (вернёт false).
	//! 3) Истечение таймаута (вернёт false).
	//! До вызова функции мьютекс должен быть захвачен.
	//! Во время ожидания мьютекс свободен.
	//! После вызова мьютекс снова станет захвачен.
	//! @return Возвращает true, если условие pred в результате ожидания было выполнено.
	template<typename L, typename P> bool WaitMs(Lock<L>& lock, ulong64 timeout, P pred)
	{
		return WaitMs(lock.Primitive(), timeout, pred);
	}

	//! Разбудить один из потоков, ожидающих на этом экземпляре SeparateCondVar.
	//! Способ выбора пробуждаемого потока не определён и зависит от реализации.
	void Notify();

	//! Разбудить все потоки, ожидающие на текущем экземпляре SeparateCondVar.
	void NotifyAll();

private:
	//! Ждёт, пока не произойдёт одно из следующих событий:
	//! 1) NotifyAll или Notify, при котором будет выбран текущий поток (вернёт true).
	//! 2) Ложное пробуждение (вернёт true).
	//! До вызова функции мьютекс должен быть захвачен.
	//! Во время выполнения этой функции мьютекс будет свободным.
	//! После вызова мьютекс снова станет захвачен.
	bool wait(Mutex& lock);

	//! Ждёт, пока не произойдёт одно из следующих событий:
	//! 1) NotifyAll или Notify, при котором будет выбран текущий поток (вернёт true).
	//! 2) Ложное пробуждение (вернёт true).
	//! 3) Истечение таймаута (вернёт false).
	//! До вызова функции мьютекс должен быть захвачен.
	//! Во время выполнения этой функции мьютекс будет свободным.
	//! После вызова мьютекс снова станет захвачен.
	bool waitUntil(Mutex& lock, ulong64 absTimeMs);

	SeparateCondVar(const SeparateCondVar&) = delete;
	SeparateCondVar& operator=(const SeparateCondVar&) = delete;
};

class CondVar: private Mutex
{
	template<typename T> friend class Lock;

	SeparateCondVar mCondVar;

	CondVar(const CondVar&) = delete;
	CondVar& operator=(const CondVar&) = delete;
public:
	CondVar() {}

	//! Ждёт, пока не произойдёт одно из следующих событий:
	//! 1) NotifyAll или Notify, при котором будет выбран текущий поток, и при этом выполнится условие pred (вернёт true).
	//! 2) Прерывание текущего потока через Interrupt (вернёт false).
	//! До вызова функции мьютекс должен быть захвачен.
	//! Во время ожидания мьютекс свободен.
	//! После вызова мьютекс снова станет захвачен.
	//! @return Возвращает true, если условие pred в результате ожидания было выполнено.
	template<typename P> bool Wait(P pred)
	{
		return mCondVar.Wait(*this, pred);
	}

	//! Ждёт, пока не произойдёт одно из следующих событий:
	//! 1) NotifyAll или Notify, при котором будет выбран текущий поток, и при этом выполнится условие pred (вернёт true).
	//! 2) Прерывание текущего потока через Interrupt (вернёт false).
	//! 3) Истечение таймаута (вернёт false).
	//! До вызова функции мьютекс должен быть захвачен.
	//! Во время ожидания мьютекс свободен.
	//! После вызова мьютекс снова станет захвачен.
	//! @return Возвращает true, если условие pred в результате ожидания было выполнено.
	template<typename P> bool WaitMs(ulong64 timeout, P pred)
	{
		return mCondVar.WaitMs(*this, timeout, pred);
	}

	//! Разбудить один из потоков, ожидающих на этом экземпляре CondVar.
	//! Способ выбора пробуждаемого потока не определён и зависит от реализации.
	void Notify() {mCondVar.Notify();}

	//! Разбудить все потоки, ожидающие на текущем экземпляре CondVar.
	void NotifyAll() {mCondVar.NotifyAll();}

	forceinline Mutex& GetMutex() {return *this;}
	forceinline SeparateCondVar& GetSeparateCondVar() {return mCondVar;}
};
#else
class SeparateCondVar;
class CondVar;
#endif

}
using Concurrency::SeparateCondVar;
using Concurrency::CondVar;

}

INTRA_WARNING_POP
