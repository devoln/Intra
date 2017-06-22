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
	enum {DATA_SIZE = 48}; //TODO: ������ ������ ������ std::condition_variable �� ������ ���������� ��� ���� �� ������������
#elif(INTRA_LIBRARY_MUTEX == INTRA_LIBRARY_MUTEX_WinAPI)
#ifdef INTRA_DROP_XP_SUPPORT
	enum {DATA_SIZE = sizeof(void*)};
#else
	enum {DATA_SIZE = sizeof(void*) + sizeof(Mutex) + sizeof(void*)*3*2 + sizeof(int)*2 + sizeof(int)};
#endif
#elif(INTRA_PLATFORM_OS == INTRA_PLATFORM_OS_Linux || INTRA_PLATFORM_OS == INTRA_PLATFORM_OS_Android)
	enum {DATA_SIZE = 48};
#elif(INTRA_PLATFORM_OS == INTRA_PLATFORM_OS_MacOS)
	enum {DATA_SIZE = 48}; //TODO: �� ���������
#elif(INTRA_PLATFORM_OS == INTRA_PLATFORM_OS_iOS)
	enum {DATA_SIZE = 48}; //TODO: �� ���������
#else
	enum {DATA_SIZE = 48}; //TODO: �� ���������
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

	//! ��������� �� ����������.
	//! CONDITION_VARIABLE* ��� WinAPI ��� Cpp11 ���������� �� Windows Vista+
	//! null �� Windows XP ��� WinAPI ����������
	//! pthread_cond_t* �� ��������� ����������.
	AnyPtr NativeHandle();

	//! ���, ���� �� ��������� ���� �� ��������� �������:
	//! 1) NotifyAll ��� Notify, ��� ������� ����� ������ ������� �����, � ��� ���� ���������� ������� pred (����� true).
	//! 2) ���������� �������� ������ ����� Interrupt (����� false).
	//! �� ������ ������� ������� ������ ���� ��������.
	//! �� ����� �������� ������� ��������.
	//! ����� ������ ������� ����� ������ ��������.
	//! @return ���������� true, ���� ������� pred � ���������� �������� ���� ���������.
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

	//! ���, ���� �� ��������� ���� �� ��������� �������:
	//! 1) NotifyAll ��� Notify, ��� ������� ����� ������ ������� �����, � ��� ���� ���������� ������� pred (����� true).
	//! 2) ���������� �������� ������ ����� Interrupt (����� false).
	//! �� ������ ������� ������� ������ ���� ��������.
	//! �� ����� �������� ������� ��������.
	//! ����� ������ ������� ����� ������ ��������.
	//! @return ���������� true, ���� ������� pred � ���������� �������� ���� ���������.
	template<typename L, typename P> bool Wait(Lock<L>& lock, P pred)
	{
		return Wait(lock.Primitive(), pred);
	}

	//! ���, ���� �� ��������� ���� �� ��������� �������:
	//! 1) NotifyAll ��� Notify, ��� ������� ����� ������ ������� �����, � ��� ���� ���������� ������� pred (����� true).
	//! 2) ���������� �������� ������ ����� Interrupt (����� false).
	//! 3) ��������� �������� (����� false).
	//! �� ������ ������� ������� ������ ���� ��������.
	//! �� ����� �������� ������� ��������.
	//! ����� ������ ������� ����� ������ ��������.
	//! @return ���������� true, ���� ������� pred � ���������� �������� ���� ���������.
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

	//! ���, ���� �� ��������� ���� �� ��������� �������:
	//! 1) NotifyAll ��� Notify, ��� ������� ����� ������ ������� �����, � ��� ���� ���������� ������� pred (����� true).
	//! 2) ���������� �������� ������ ����� Interrupt (����� false).
	//! 3) ��������� �������� (����� false).
	//! �� ������ ������� ������� ������ ���� ��������.
	//! �� ����� �������� ������� ��������.
	//! ����� ������ ������� ����� ������ ��������.
	//! @return ���������� true, ���� ������� pred � ���������� �������� ���� ���������.
	template<typename L, typename P> bool WaitMs(Lock<L>& lock, ulong64 timeout, P pred)
	{
		return WaitMs(lock.Primitive(), timeout, pred);
	}

	//! ��������� ���� �� �������, ��������� �� ���� ���������� SeparateCondVar.
	//! ������ ������ ������������� ������ �� �������� � ������� �� ����������.
	void Notify();

	//! ��������� ��� ������, ��������� �� ������� ���������� SeparateCondVar.
	void NotifyAll();

private:
	//! ���, ���� �� ��������� ���� �� ��������� �������:
	//! 1) NotifyAll ��� Notify, ��� ������� ����� ������ ������� ����� (����� true).
	//! 2) ������ ����������� (����� true).
	//! �� ������ ������� ������� ������ ���� ��������.
	//! �� ����� ���������� ���� ������� ������� ����� ���������.
	//! ����� ������ ������� ����� ������ ��������.
	bool wait(Mutex& lock);

	//! ���, ���� �� ��������� ���� �� ��������� �������:
	//! 1) NotifyAll ��� Notify, ��� ������� ����� ������ ������� ����� (����� true).
	//! 2) ������ ����������� (����� true).
	//! 3) ��������� �������� (����� false).
	//! �� ������ ������� ������� ������ ���� ��������.
	//! �� ����� ���������� ���� ������� ������� ����� ���������.
	//! ����� ������ ������� ����� ������ ��������.
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

	//! ���, ���� �� ��������� ���� �� ��������� �������:
	//! 1) NotifyAll ��� Notify, ��� ������� ����� ������ ������� �����, � ��� ���� ���������� ������� pred (����� true).
	//! 2) ���������� �������� ������ ����� Interrupt (����� false).
	//! �� ������ ������� ������� ������ ���� ��������.
	//! �� ����� �������� ������� ��������.
	//! ����� ������ ������� ����� ������ ��������.
	//! @return ���������� true, ���� ������� pred � ���������� �������� ���� ���������.
	template<typename P> bool Wait(P pred)
	{
		return mCondVar.Wait(*this, pred);
	}

	//! ���, ���� �� ��������� ���� �� ��������� �������:
	//! 1) NotifyAll ��� Notify, ��� ������� ����� ������ ������� �����, � ��� ���� ���������� ������� pred (����� true).
	//! 2) ���������� �������� ������ ����� Interrupt (����� false).
	//! 3) ��������� �������� (����� false).
	//! �� ������ ������� ������� ������ ���� ��������.
	//! �� ����� �������� ������� ��������.
	//! ����� ������ ������� ����� ������ ��������.
	//! @return ���������� true, ���� ������� pred � ���������� �������� ���� ���������.
	template<typename P> bool WaitMs(ulong64 timeout, P pred)
	{
		return mCondVar.WaitMs(*this, timeout, pred);
	}

	//! ��������� ���� �� �������, ��������� �� ���� ���������� CondVar.
	//! ������ ������ ������������� ������ �� �������� � ������� �� ����������.
	void Notify() {mCondVar.Notify();}

	//! ��������� ��� ������, ��������� �� ������� ���������� CondVar.
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
