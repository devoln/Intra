#pragma once

#include "Mutex.h"
#include "Lock.h"
#include "Thread.h"
#include "Synchronized.h"
#include "Container/Sequential/Array.h"

#include "System/DateTime.h"

INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS

INTRA_BEGIN
namespace Concurrency {

#if(INTRA_LIBRARY_MUTEX != INTRA_LIBRARY_MUTEX_None)
//! Synchronization primitive useful for waiting for some condition.
class SeparateCondVar
{
private:
public:
#if(INTRA_LIBRARY_MUTEX == INTRA_LIBRARY_MUTEX_Cpp11)
	enum {DATA_SIZE = 48}; //TODO: find exact size of std::condition_variable on different platforms, or at least maximum size
#elif(INTRA_LIBRARY_MUTEX == INTRA_LIBRARY_MUTEX_WinAPI)
#ifdef INTRA_DROP_XP_SUPPORT
	enum {DATA_SIZE = sizeof(void*)};
#else
	enum {DATA_SIZE = sizeof(void*) + sizeof(Mutex) + sizeof(void*)*3*2 + sizeof(int)*2 + sizeof(size_t)};
#endif
#elif(INTRA_PLATFORM_OS == INTRA_PLATFORM_OS_Linux || INTRA_PLATFORM_OS == INTRA_PLATFORM_OS_Android)
	enum {DATA_SIZE = 48};
#elif(INTRA_PLATFORM_OS == INTRA_PLATFORM_OS_MacOS)
	enum {DATA_SIZE = 48}; //TODO: not tested
#elif(INTRA_PLATFORM_OS == INTRA_PLATFORM_OS_iOS)
	enum {DATA_SIZE = 48}; //TODO: not tested
#else
	enum {DATA_SIZE = 48}; //TODO: not tested
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

	//! Pointer to the implementation.
	//! CONDITION_VARIABLE* for WinAPI or Cpp11 implementations on Windows Vista+
	//! null on Windows XP for WinAPI implementation
	//! pthread_cond_t* for other platforms.
	AnyPtr NativeHandle();

	//! Wait for one of the following events:
	//! 1) Call to NotifyAll or Notify wakes up the current thread, and pred is true.
	//! 2) Call to Interrupt (in this case returns false).
	//! Before calling this method the mutex must be acquired.
	//! The mutex is free while waiting.
	//! When Wait returns the mutex becomes acquired again.
	//! @return true, if pred became true.
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

	//! Wait for one of the following events:
	//! 1) Call to NotifyAll or Notify wakes up the current thread, and pred is true.
	//! 2) Call to Interrupt (in this case returns false).
	//! Before calling this method the lock must be acquired.
	//! The lock is free while waiting.
	//! When Wait returns lock will become acquired again.
	//! @return true, if pred became true.
	template<typename L, typename P> bool Wait(Lock<L>& lock, P pred)
	{
		return Wait(lock.Primitive(), pred);
	}

	//! Wait for one of the following events:
	//! 1) Call to NotifyAll or Notify wakes up the current thread, and pred is true.
	//! 2) Call to Interrupt (in this case returns false).
	//! 3) timeout expires (in this case returns false)
	//! Before calling this method the mutex must be acquired.
	//! The mutex is free while waiting.
	//! When WaitMs returns mutex will become acquired again.
	//! @return true, if pred became true.
	template<typename P> bool WaitMs(Mutex& mutex, uint64 timeout, P pred)
	{
		const uint64 absTimeMs = System::DateTime::AbsTimeMs() + timeout;
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

	//! Wait for one of the following events:
	//! 1) Call to NotifyAll or Notify wakes up the current thread, and pred is true.
	//! 2) Call to Interrupt (in this case returns false).
	//! 3) timeout expires (in this case returns false)
	//! Before calling this method the lock must be acquired.
	//! The lock is free while waiting.
	//! When Wait returns lock will become acquired again.
	//! @return true, if pred became true.
	template<typename L, typename P> bool WaitMs(Lock<L>& lock, uint64 timeout, P pred)
	{
		return WaitMs(lock.Primitive(), timeout, pred);
	}

	//! Wake up one of threads waiting on this SeparateCondVar to check pred.
	//! The way of selecting a thread to be waked is undefined and depends on implementation.
	void Notify();

	//! Wake up all threads waiting on this SeparateCondVar to check pred.
	void NotifyAll();

private:
	//! Wait for one of the following events:
	//! 1) Call to NotifyAll or Notify wakes up the current thread.
	//! 2) Call to Interrupt.
	//! 3) Spurious wake up.
	//! Before calling this method the mutex must be acquired.
	//! The mutex is free while waiting.
	//! When wait returns mutex will become acquired again.
	bool wait(Mutex& lock);

	//! Wait for one of the following events:
	//! 1) Call to NotifyAll or Notify wakes up the current thread.
	//! 2) Call to Interrupt.
	//! 3) Spurious wake up.
	//! 4) absTimeMs expires.
	//! Before calling this method the mutex must be acquired.
	//! The mutex is free while waiting.
	//! When wait returns mutex will become acquired again.
	bool waitUntil(Mutex& lock, uint64 absTimeMs);

	SeparateCondVar(const SeparateCondVar&) = delete;
	SeparateCondVar& operator=(const SeparateCondVar&) = delete;
};

//! Combines conditional variable and mutex into one class.
class CondVar: private Mutex
{
	template<typename T> friend class Lock;

	SeparateCondVar mCondVar;

	CondVar(const CondVar&) = delete;
	CondVar& operator=(const CondVar&) = delete;
public:
	CondVar() {}

	//! Wait for one of the following events:
	//! 1) Call to NotifyAll or Notify wakes up the current thread, and pred is true.
	//! 2) Call to Interrupt (in this case returns false).
	//! @return true, if pred became true.
	template<typename P> bool Wait(P pred)
	{
		return mCondVar.Wait(*this, pred);
	}

	//! Wait for one of the following events:
	//! 1) Call to NotifyAll or Notify wakes up the current thread, and pred is true.
	//! 2) Call to Interrupt (in this case returns false).
	//! 3) timeout expires (in this case returns false).
	//! @return true, if pred became true.
	template<typename P> bool WaitMs(uint64 timeout, P pred)
	{
		return mCondVar.WaitMs(*this, timeout, pred);
	}

	//! Wake up one of threads waiting on this SeparateCondVar to check pred.
	//! The way of selecting a thread to be waked is undefined and depends on implementation.
	void Notify() {mCondVar.Notify();}

	//! Wake up all threads waiting on this SeparateCondVar to check pred.
	void NotifyAll() {mCondVar.NotifyAll();}

	//! Get internal mutex
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
