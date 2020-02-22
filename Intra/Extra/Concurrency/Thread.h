#pragma once

#include "Core/Range/StringView.h"

#include "Utils/Delegate.h"
#include "Utils/Unique.h"

#undef Yield

#define INTRA_LIBRARY_THREAD_None 0
#define INTRA_LIBRARY_THREAD_WinAPI 1
#define INTRA_LIBRARY_THREAD_Cpp11 2
#define INTRA_LIBRARY_THREAD_PThread 3

#ifndef INTRA_LIBRARY_THREAD

#if(INTRA_PLATFORM_OS == INTRA_PLATFORM_OS_Windows)

#define INTRA_LIBRARY_THREAD INTRA_LIBRARY_THREAD_WinAPI

#elif(INTRA_PLATFORM_OS == INTRA_PLATFORM_OS_Emscripten)

#ifdef __EMSCRIPTEN_PTHREADS__
#define INTRA_LIBRARY_THREAD INTRA_LIBRARY_THREAD_PThread
#else
#define INTRA_LIBRARY_THREAD INTRA_LIBRARY_THREAD_None
#endif

#else
#define INTRA_LIBRARY_THREAD INTRA_LIBRARY_THREAD_PThread
#endif

#endif

INTRA_BEGIN
#if(INTRA_LIBRARY_THREAD != INTRA_LIBRARY_THREAD_None)
class Thread
{
public:
	struct Data;
	typedef Data* Handle;
private:
	Unique<Data> mHandle;
public:
	//! HANDLE in windows.
	//! pthread_t* in other OSes.
	typedef void* NativeHandle;

	typedef Delegate<void()> Func;

	Thread(null_t=null) noexcept;
	Thread(Thread&& rhs) noexcept;
	explicit Thread(Func func);
	explicit Thread(StringView name, Func func):
		Thread(Move(func)) {SetName(name);}

	//! Destructor calls Interrupt and Join.
	~Thread();
	Thread& operator=(Thread&& rhs);

	//! Block running of current thread until this thread finishes or timeout expires or Interrupt on current thread is called.
	//! @param timeout measured in milliseconds.
	//! @return true, if this thread finished its execution.
	bool JoinMs(uint64 timeout);

	//! Block running of current thread until this thread finishes or Interrupt on current thread is called.
	//! @return true, if this thread finished its execution.
	bool Join();

	//! This thread object becomes null,
	//! its thread continues running independently.
	void Detach();

	//! Set the interruption flag for this thread.
	//! If the interrption mechanism was not disabled via DisableInterruption()
	//! or via #define INTRA_THREAD_NO_FULL_INTERRUPT, then
	//! interrupts waiting on Sleep, Join/JoinMs and CondVar::Wait/WaitMs causing them to return false.
	void Interrupt();

	//! @return if interruption flag is set for this thread.
	bool IsInterrupted() const;

#ifndef INTRA_THREAD_NO_FULL_INTERRUPT
	//! Allow method Interrupt() to interrupt waiting for this thread.
	forceinline void EnableInterruption() {allowInterruption(true);}
#endif
	//! Disallow method Interrupt() to interrupt waiting for this thread.
	forceinline void DisableInterruption()
	{
#ifndef INTRA_THREAD_NO_FULL_INTERRUPT
		allowInterruption(false);
#endif
	}

	//! @return true, if this thread supports interruption.
	bool IsInterruptionEnabled() const;

	//! @return true if this thread is running at the moment of calling.
	bool IsRunning() const;

	//! Set thread name, useful for debugging.
	void SetName(StringView name);

	//! @return thread name that have been set via SetName.
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
	constexpr forceinline TThisThread() {}
	TThisThread(const TThisThread&) = delete;
	TThisThread& operator=(const TThisThread&) = delete;

	//! Set interruption flag indicating that current thread should finish its execution.
	//! Works only for threads created with Thread class with interruption enabled.
	static void Interrupt();

	static bool IsInterrupted();

	//! OS handle of currently running thread.
	static Thread::NativeHandle NativeHandle();

	//! Name of currently running thread (set via SetName method of Thread class)
	static StringView Name();

	//! Provide a hint to the OS thread scheduler, allowing other threads to run.
	static void Yield();

	//! Wait until milliseconds expire or Interrupt is called on this thread.
	static bool Sleep(uint64 milliseconds);

	#ifndef INTRA_THREAD_NO_FULL_INTERRUPT
	//! Allow Interrupt method to interrupt waiting of current thread.
	static forceinline void EnableInterruption() {allowInterruption(true);}
#endif
	//! Disallow Interrupt method to interrupt waiting of current thread.
	static forceinline void DisableInterruption()
	{
#ifndef INTRA_THREAD_NO_FULL_INTERRUPT
		allowInterruption(false);
#endif
	}

	//! @return true if current thread supports interruption.
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
#else
class Thread;
struct TThisThread;
#endif
INTRA_END
