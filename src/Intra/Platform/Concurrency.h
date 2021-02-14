#pragma once

#include "Intra/Range/StringView.h"

#include "IntraX/Utils/Delegate.h"
#include "IntraX/Utils/Unique.h"

#if __has_include(<pthread.h>)
#include <pthread.h>
#ifdef _MSC_VER
#pragma comment(lib, "pthread.lib")
#endif
#endif

#ifdef _WIN32
struct _RTL_CRITICAL_SECTION;
struct _RTL_SRWLOCK;
struct _RTL_CONDITION_VARIABLE;
#endif

namespace Intra { INTRA_BEGIN

class Thread
{
public:
	struct Data;
	using Handle = Data*;
private:
	Unique<Data> mHandle;
public:
	/// HANDLE in windows.
	/// pthread_t* in other OSes.
	using NativeHandle = void*;

	using Func = Delegate<void()>;

	Thread(decltype(nullptr) = nullptr) noexcept;
	Thread(Thread&& rhs) noexcept;
	explicit Thread(Func func);
	explicit Thread(StringView name, Func func):
		Thread(Move(func)) {SetName(name);}

	/// Destructor calls Interrupt and Join.
	~Thread();
	Thread& operator=(Thread&& rhs);

	/// Block running of current thread until this thread finishes or timeout expires or Interrupt on current thread is called.
	/// @param timeout measured in milliseconds.
	/// @return true, if this thread finished its execution.
	bool JoinMs(uint64 timeout);

	/// Block running of current thread until this thread finishes or Interrupt on current thread is called.
	/// @return true, if this thread finished its execution.
	bool Join();

	/// This thread object becomes nullptr,
	/// its thread continues running independently.
	void Detach();

	/// Set the interruption flag for this thread.
	/// If the interrption mechanism was not disabled via DisableInterruption()
	/// or via #define INTRA_THREAD_NO_FULL_INTERRUPT, then
	/// interrupts waiting on Sleep, Join/JoinMs and CondVar::Wait/WaitMs causing them to return false.
	void Interrupt();

	/// @return if interruption flag is set for this thread.
	bool IsInterrupted() const;

#ifndef INTRA_THREAD_NO_FULL_INTERRUPT
	/// Allow method Interrupt() to interrupt waiting for this thread.
	void EnableInterruption() {allowInterruption(true);}
#endif
	/// Disallow method Interrupt() to interrupt waiting for this thread.
	void DisableInterruption()
	{
#ifndef INTRA_THREAD_NO_FULL_INTERRUPT
		allowInterruption(false);
#endif
	}

	/// @return true, if this thread supports interruption.
	bool IsInterruptionEnabled() const;

	/// @return true if this thread is running at the moment of calling.
	bool IsRunning() const;

	/// Set thread name, useful for debugging.
	void SetName(StringView name);

	/// @return thread name that have been set via SetName.
	StringView Name() const;

	/** @return Platform dependent API thread handle:
		HANDLE for Windows (created with _beginthreadex\CreateThread)
		pthread* for other platforms.
		This handle becomes invalid after Detach or thread destruction.
	*/
	NativeHandle GetNativeHandle() const;

private:
	Thread(const Thread& rhs) = delete;
	Thread& operator=(const Thread&) = delete;

#ifndef INTRA_THREAD_NO_FULL_INTERRUPT
	void allowInterruption(bool allow);
#endif
};

class SeparateCondVar;
class Mutex;

struct TThisThread
{
	constexpr TThisThread() {}
	TThisThread(const TThisThread&) = delete;
	TThisThread& operator=(const TThisThread&) = delete;

	/// Set interruption flag indicating that current thread should finish its execution.
	/// Works only for threads created with Thread class with interruption enabled.
	static void Interrupt();

	static bool IsInterrupted();

	/// OS handle of currently running thread.
	static Thread::NativeHandle NativeHandle();

	/// Name of currently running thread (set via SetName method of Thread class)
	static StringView Name();

	/// Provide a hint to the OS thread scheduler, allowing other threads to run.
	static void Yield();

	/// Wait until milliseconds expire or Interrupt is called on this thread.
	static bool Sleep(uint64 milliseconds);

#ifndef INTRA_THREAD_NO_FULL_INTERRUPT
	/// Allow Interrupt method to interrupt waiting of current thread.
	static void EnableInterruption() {allowInterruption(true);}
#endif
	/// Disallow Interrupt method to interrupt waiting of current thread.
	static void DisableInterruption()
	{
#ifndef INTRA_THREAD_NO_FULL_INTERRUPT
		allowInterruption(false);
#endif
	}

	/// @return true if current thread supports interruption.
	static bool IsInterruptionEnabled();

private:
#if !defined(INTRA_THREAD_NO_FULL_INTERRUPT) || defined(INTRA_DEBUG)
	friend class SeparateCondVar;
	static void onWait(SeparateCondVar* cv, Mutex* mutex);
#endif
#ifndef INTRA_THREAD_NO_FULL_INTERRUPT
	static void allowInterruption(bool allow);
#endif

#ifdef INTRA_DEBUG
	static Thread::Handle getHandle();
#endif
};
extern const TThisThread ThisThread;


#if __has_include(<pthread.h>)
struct PosixMutex
{
	PosixMutex(const PosixMutex&) = delete;
	PosixMutex& operator=(const PosixMutex&) = delete;

	enum class TRecursive {Recursive};

	PosixMutex() = default;
	explicit PosixMutex(TRecursive): mMutex(PTHREAD_RECURSIVE_MUTEX_INITIALIZER) {}

	~PosixMutex() {pthread_mutex_destroy(&mMutex);}
	void lock() {pthread_mutex_lock(&mMutex);}
	bool try_lock() {return pthread_mutex_trylock(&mMutex) != 0;}
	void unlock() {pthread_mutex_unlock(&mMutex);}

	pthread_mutex_t* Handle() {return &mMutex;}

private:
	pthread_mutex_t mMutex = PTHREAD_ERRORCHECK_MUTEX_INITIALIZER;
};
struct PosixCondVar
{
	PosixCondVar(const PosixCondVar&) = delete;
	PosixCondVar& operator=(const PosixCondVar&) = delete;

	PosixCondVar() = default;
	bool wait_for(pthread_mutex_t* lock, uint32 timeoutMs) {return z_D::SleepConditionVariableCS(Handle(), lock, timeoutMs) != 0;}
	bool wait_for(_RTL_SRWLOCK* lock, uint32 timeoutMs) {return z_D::SleepConditionVariableSRW(Handle(), lock, timeoutMs, 0) != 0;}
	void notify_one() {z_D::WakeConditionVariable(Handle());}
	void notify_all() {z_D::WakeAllConditionVariable(Handle());}

	bool wait_for(WinCriticalSection& lock, uint32 timeoutMs) {return wait_for(lock.Handle(), timeoutMs);}
	bool wait_for(WinSRWLock& lock, uint32 timeoutMs) {return wait_for(lock.Handle(), timeoutMs);}
	void wait(_RTL_CRITICAL_SECTION* lock) {if(!wait_for(lock, 0xFFFFFFFF)) INTRA_FATAL_ERROR("Wait failed!");}
	void wait(_RTL_SRWLOCK* lock) {if(!wait_for(lock, 0xFFFFFFFF)) INTRA_FATAL_ERROR("Wait failed!");}
	void wait(WinCriticalSection& lock) {wait(lock.Handle());}
	void wait(WinSRWLock& lock) {wait(lock.Handle());}

	_RTL_CONDITIONAL_VARIABLE* Handle() {return reinterpret_cast<_RTL_CONDITIONAL_VARIABLE*>(&mCondVar);}

private:
	void* mCondVar = nullptr;
};
#endif

#ifdef _WIN32
namespace z_D { extern "C" {
INTRA_DLL_IMPORT int INTRA_WINAPI InitializeCriticalSectionAndSpinCount(_RTL_CRITICAL_SECTION*, uint32 spinCount);
INTRA_DLL_IMPORT int INTRA_WINAPI TryEnterCriticalSection(_RTL_CRITICAL_SECTION*);
INTRA_DLL_IMPORT void INTRA_WINAPI EnterCriticalSection(_RTL_CRITICAL_SECTION*);
INTRA_DLL_IMPORT void INTRA_WINAPI LeaveCriticalSection(_RTL_CRITICAL_SECTION*);
INTRA_DLL_IMPORT void INTRA_WINAPI DeleteCriticalSection(_RTL_CRITICAL_SECTION*);

INTRA_DLL_IMPORT void INTRA_WINAPI ReleaseSRWLockExclusive(_RTL_SRWLOCK*);
INTRA_DLL_IMPORT void INTRA_WINAPI ReleaseSRWLockShared(_RTL_SRWLOCK*);
INTRA_DLL_IMPORT void INTRA_WINAPI AcquireSRWLockExclusive(_RTL_SRWLOCK*);
INTRA_DLL_IMPORT void INTRA_WINAPI AcquireSRWLockShared(_RTL_SRWLOCK*);
INTRA_DLL_IMPORT uint8 INTRA_WINAPI TryAcquireSRWLockExclusive(_RTL_SRWLOCK*);
INTRA_DLL_IMPORT uint8 INTRA_WINAPI TryAcquireSRWLockShared(_RTL_SRWLOCK*);

INTRA_DLL_IMPORT void INTRA_WINAPI WakeConditionVariable(_RTL_CONDITION_VARIABLE*);
INTRA_DLL_IMPORT void INTRA_WINAPI WakeAllConditionVariable(_RTL_CONDITION_VARIABLE*);
INTRA_DLL_IMPORT int INTRA_WINAPI SleepConditionVariableCS(_RTL_CONDITION_VARIABLE*, _RTL_CRITICAL_SECTION*, uint32 ms);
INTRA_DLL_IMPORT int INTRA_WINAPI SleepConditionVariableSRW(_RTL_CONDITION_VARIABLE*, _RTL_SRWLOCK*, uint32 ms, uint32 flags);
}}

struct WinCriticalSection
{
	WinCriticalSection(const WinCriticalSection&) = delete;
	WinCriticalSection& operator=(const WinCriticalSection&) = delete;

	WinCriticalSection() {z_D::InitializeCriticalSectionAndSpinCount(&mCriticalSection, 4000);}
	~WinCriticalSection() {z_D::DeleteCriticalSection(Handle());}
	void lock() {z_D::EnterCriticalSection(Handle());}
	bool try_lock() {return z_D::TryEnterCriticalSection(Handle()) != 0;}
	void unlock() {z_D::LeaveCriticalSection(Handle());}

	_RTL_CRITICAL_SECTION* Handle() {return reinterpret_cast<_RTL_CRITICAL_SECTION*>(&mCriticalSection);}

private:
	struct
	{
		void* DebugInfo;
		long LockCount;
		long RecursionCount;
		void* OwningThread;
		void* LockSemaphore;
		size_t SpinCount;
	} mCriticalSection;
};

struct WinSRWLock
{
	WinSRWLock(const WinSRWLock&) = delete;
	WinSRWLock& operator=(const WinSRWLock&) = delete;

	WinSRWLock() = default;
	void lock() {z_D::AcquireSRWLockExclusive(Handle());}
	bool try_lock() {return z_D::TryAcquireSRWLockExclusive(Handle()) != 0;}
	void unlock() {z_D::ReleaseSRWLockExclusive(Handle());}
	void lock_shared() {z_D::AcquireSRWLockShared(Handle());}
	bool try_lock_shared() {return z_D::TryAcquireSRWLockShared(Handle()) != 0;}
	void unlock_shared() {z_D::ReleaseSRWLockShared(Handle());}

	_RTL_SRWLOCK* Handle() {return reinterpret_cast<_RTL_SRWLOCK*>(&mSRWLock);}

private:
	void* mSRWLock = nullptr;
};

struct WinCondVar
{
	WinCondVar(const WinCondVar&) = delete;
	WinCondVar& operator=(const WinCondVar&) = delete;

	WinCondVar() = default;
	bool wait_for(_RTL_CRITICAL_SECTION* lock, uint32 timeoutMs) {return z_D::SleepConditionVariableCS(Handle(), lock, timeoutMs) != 0;}
	bool wait_for(_RTL_SRWLOCK* lock, uint32 timeoutMs) {return z_D::SleepConditionVariableSRW(Handle(), lock, timeoutMs, 0) != 0;}
	void notify_one() {z_D::WakeConditionVariable(Handle());}
	void notify_all() {z_D::WakeAllConditionVariable(Handle());}

	bool wait_for(WinCriticalSection& lock, uint32 timeoutMs) {return wait_for(lock.Handle(), timeoutMs);}
	bool wait_for(WinSRWLock& lock, uint32 timeoutMs) {return wait_for(lock.Handle(), timeoutMs);}
	void wait(_RTL_CRITICAL_SECTION* lock) {if(!wait_for(lock, 0xFFFFFFFF)) INTRA_FATAL_ERROR("Wait failed!");}
	void wait(_RTL_SRWLOCK* lock) {if(!wait_for(lock, 0xFFFFFFFF)) INTRA_FATAL_ERROR("Wait failed!");}
	void wait(WinCriticalSection& lock) {wait(lock.Handle());}
	void wait(WinSRWLock& lock) {wait(lock.Handle());}

	_RTL_CONDITIONAL_VARIABLE* Handle() {return reinterpret_cast<_RTL_CONDITIONAL_VARIABLE*>(&mCondVar);}

private:
	void* mCondVar = nullptr;
};
#endif



} INTRA_END
