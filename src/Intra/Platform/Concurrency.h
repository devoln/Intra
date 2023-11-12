#pragma once

#include <Intra/Platform/Toolchain.h>
#include <Intra/Range/StringView.h>
#include <Intra/TypeErasure.h>
#include <Intra/Container/Ownership.h>
#include <Intra/Platform/DateTime.h>
#include <Intra/Platform/Atomic.h>

namespace Intra { INTRA_BEGIN

struct ThisThread
{
	ThisThread() = delete;

	/// Name of currently running thread (set via SetName method of Thread class)
	static INTRA_FORCEINLINE String Name() {return currentThreadName;}

	static INTRA_FORCEINLINE void SetName(String name)
	{
#ifdef _MSC_VER
		z_D::SetWinThreadDebugName(z_D::GetCurrentThread(), name);
#endif
		currentThreadName = INTRA_MOVE(name);
	}

	static INTRA_FORCEINLINE constexpr void BusyWaitCpuPause()
	{
		if(IsConstantEvaluated()) return;
#ifdef _MSC_VER
#if defined(__i386__) || defined(__amd64__)
		z_D::_mm_pause();
#else
		// TODO: ARM and ARM64 implementation
#endif
#elif defined(__i386__) || defined(__amd64__)
		__asm__ __volatile__("pause" ::: "memory");
#else
		// TODO: implement for other architectures
#endif
	}

	static void Sleep(TimeDelta time)
	{
#ifdef _WIN32
		z_D::Sleep(uint32(Min(Max(time.IntMilliseconds(), 0), MaxValueOf<uint32>)));
#else
		const auto ts = time.To<z_D::timespec>();
		z_D::nanosleep(Unsafe(&ts), nullptr);
#endif
	}

private:
	static inline thread_local String currentThreadName = {};
};


#if __has_include(<pthread.h>)
class PosixThread
{
public:
	PosixThread() = default;
	PosixThread(const PosixThread& rhs) = delete;
	PosixThread& operator=(const PosixThread&) = delete;
	PosixThread(PosixThread&& rhs) noexcept = default;
	PosixThread& operator=(PosixThread&&) = default;

	explicit PosixThread(PCallable<void()> threadFunc, pthread_attr_t attr)
	{
		pthread_create(&mThread.Id, &attr, threadProc, threadFunc.release());
	}

	explicit PosixThread(PCallable<void()> threadFunc, size_t reservedStackSize = 0):
		PosixThread(INTRA_MOVE(threadFunc), [] {
			z_D::pthread_attr_t attr;
			z_D::pthread_attr_init(&attr);
			if(reservedStackSize) z_D::pthread_attr_setstacksize(&attr, reservedStackSize);
			return attr;
		}()) {}

	~PosixThread() {INTRA_PRECONDITION(!IsJoinable());}

	[[nodiscard]] INTRA_FORCEINLINE bool IsJoinable() const {return bool(mThread);}

	/// Block running of current thread until this thread finishes
	INTRA_FORCEINLINE void Join() {z_D::pthread_join(mThread, nullptr); mThread = {};}

	/// This thread continues running independently of this object
	INTRA_FORCEINLINE void Detach() {z_D::pthread_detach(mThread); mThread = {};}

	[[nodiscard]] INTRA_FORCEINLINE pthread_t PThread() const {return mThread;}

	[[nodiscard]] INTRA_FORCEINLINE auto NativeHandle() const
	{
#ifdef pthread_getw32threadhandle_np
		return z_D::pthread_getw32threadhandle_np(mThread);
#else
		return z_D::pthread_gethandle(mThread);
#endif
	}

	static void SetCurrentThreadName(StringView name)
	{
		constexpr int maxLen = (TargetOS == OperatingSystem::MacOS || TargetOS == OperatingSystem::IOS)? 64: 16;
		char nameBuf[maxLen] = {};
		z_D::memcpy(nameBuf, name.Data(), Min(name.Length(), sizeof(nameBuf) - 1));
#if defined(__linux__) || defined(_WIN32)
		z_D::pthread_setname_np(z_D::pthread_self(), nameBuf);
#elif defined(__FreeBSD__)
		z_D::pthread_set_name_np(z_D::pthread_self(), nameBuf);
#elif defined(__APPLE__)
		z_D::pthread_setname_np(nameBuf);
#endif
	}

private:
	HandleMovableWrapper<pthread_t> mThread;

	static void* threadProc(void* lpParam)
	{
		Unique<ICallable<void()>> threadFunc(static_cast<ICallable<void()>*>(lpParam));
		(*threadFunc)();
		return nullptr;
	}
};

struct PosixMutex
{
	PosixMutex(const PosixMutex&) = delete;
	PosixMutex& operator=(const PosixMutex&) = delete;

#if INTRA_ALLOW_NON_PORTABLE_PTHREAD_CODE
	PosixMutex() = default;
#else
	PosixMutex()
	{
#ifdef INTRA_DEBUG
		z_D::pthread_mutexattr_t attr;
		z_D::pthread_mutexattr_init(Unsafe(&attr));
		z_D::pthread_mutexattr_settype(Unsafe(&attr), 2);
		z_D::pthread_mutex_init(Handle(), Unsafe(&attr));
#else
		z_D::pthread_mutex_init(Handle(), nullptr);
#endif
	}
#endif

	enum class TRecursive {Recursive};
	explicit PosixMutex(TRecursive)
#if defined(INTRA_PTHREAD_RECURSIVE_MUTEX_INITIALIZER) && INTRA_ALLOW_NON_PORTABLE_PTHREAD_CODE
		: mMutex(INTRA_PTHREAD_RECURSIVE_MUTEX_INITIALIZER) {}
#elif defined(INTRA_PTHREAD_RECURSIVE_MUTEX_INITIALIZER_NP) && INTRA_ALLOW_NON_PORTABLE_PTHREAD_CODE
		: mMutex(INTRA_PTHREAD_RECURSIVE_MUTEX_INITIALIZER_NP) {}
#else
	{
		z_D::pthread_mutexattr_t attr;
		z_D::pthread_mutexattr_init(Unsafe(&attr));
		z_D::pthread_mutexattr_settype(Unsafe(&attr), 1);
		z_D::pthread_mutex_init(Handle(), Unsafe(&attr));
	}
#endif
	~PosixMutex() {z_D::pthread_mutex_destroy(Handle());}

	void Lock() {INTRA_DEBUG_EXPECT_NO_ERROR(z_D::pthread_mutex_lock(Handle()));}
	INTRA_FORCEINLINE bool TryLock() {return z_D::pthread_mutex_trylock(Handle()) == 0;}
	void Unlock() {INTRA_DEBUG_EXPECT_NO_ERROR(z_D::pthread_mutex_unlock(Handle()));}

	INTRA_FORCEINLINE pthread_mutex_t* Handle() {return Unsafe(&mMutex);}

private:
	z_D::pthread_mutex_t mMutex
#if INTRA_ALLOW_NON_PORTABLE_PTHREAD_CODE
		=
#ifdef INTRA_DEBUG
#ifdef INTRA_PTHREAD_ERRORCHECK_MUTEX_INITIALIZER
		INTRA_PTHREAD_ERRORCHECK_MUTEX_INITIALIZER
#elif defined(INTRA_PTHREAD_ERRORCHECK_MUTEX_INITIALIZER_NP)
		INTRA_PTHREAD_ERRORCHECK_MUTEX_INITIALIZER_NP
#else
		INTRA_PTHREAD_MUTEX_INITIALIZER
#endif
#else
		INTRA_PTHREAD_MUTEX_INITIALIZER
#endif
#endif
		;
};

struct PosixRWLock
{
	PosixRWLock(const PosixRWLock&) = delete;
	PosixRWLock& operator=(const PosixRWLock&) = delete;

#if INTRA_ALLOW_NON_PORTABLE_PTHREAD_CODE
	PosixRWLock() = default;
#else
	PosixRWLock()
	{
#ifdef INTRA_DEBUG
		z_D::pthread_mutexattr_t attr;
		z_D::pthread_mutexattr_init(Unsafe(&attr));
		z_D::pthread_mutexattr_settype(Unsafe(&attr), 2);
		z_D::pthread_rwlock_init(Handle(), Unsafe(&attr));
#else
		z_D::pthread_rwlock_init(Handle(), nullptr);
#endif
	}
#endif
	INTRA_FORCEINLINE ~PosixRWLock() {z_D::pthread_rwlock_destroy(Handle());}

	void Lock() {INTRA_DEBUG_EXPECT_NO_ERROR(pthread_rwlock_wrlock(Handle()));}
	INTRA_FORCEINLINE bool Lock(Timestamp timeout)
	{
		const auto ts = timeout.SystemTime.Since1970.To<z_D::timespec>();
		return pthread_timedrwlock_wrlock(Handle(), Unsafe(&ts)) == 0;
	}
	INTRA_FORCEINLINE bool TryLock() {return pthread_rwlock_trywrlock(Handle()) == 0;}
	void Unlock() {INTRA_DEBUG_EXPECT_NO_ERROR(pthread_rwlock_unlock(Handle()));}

	void LockShared() {INTRA_DEBUG_EXPECT_NO_ERROR(pthread_rwlock_rdlock(Handle()));}
	bool LockShared(Timestamp timeout)
	{
		const auto ts = timeout.SystemTime.Since1970.To<z_D::timespec>();
		return pthread_timedrwlock_rdlock(Handle(), Unsafe(&ts)) == 0;
	}
	INTRA_FORCEINLINE bool TryLockShared() {return pthread_rwlock_tryrdlock(Handle()) == 0;}
	INTRA_FORCEINLINE void UnlockShared() {Unlock();}

	INTRA_FORCEINLINE pthread_rwlock_t* Handle() {return Unsafe(&mRWLock);}

private:
	z_D::pthread_rwlock_t mRWLock
#if INTRA_ALLOW_NON_PORTABLE_PTHREAD_CODE
		= INTRA_PTHREAD_RWLOCK_INITIALIZER
#endif
	;
};

// Uses system time clock for timed wait operations, most portable
struct PosixCondVar
{
	PosixCondVar(const PosixCondVar&) = delete;
	PosixCondVar& operator=(const PosixCondVar&) = delete;

	PosixCondVar() = default;
	INTRA_FORCEINLINE bool Wait(pthread_mutex_t* lock, Timestamp timeout)
	{
		const auto ts = timeout.SystemTime.Since1970.To<z_D::timespec>();
		return pthread_cond_timedwait(Handle(), lock, Unsafe(&ts)) == 0;
	}
	INTRA_FORCEINLINE bool Wait(pthread_mutex_t* lock, TimeDelta timeout) {return Wait(lock, Timestamp::Now() + timeout);}
	INTRA_FORCEINLINE bool Wait(PosixMutex& lock, Timestamp timeout) {return Wait(lock.Handle(), timeout);}
	INTRA_FORCEINLINE bool Wait(PosixMutex& lock, TimeDelta timeout) {return Wait(lock.Handle(), timeout);}
	
	INTRA_FORCEINLINE void NotifyOne() {pthread_cond_signal(Handle());}
	INTRA_FORCEINLINE void NotifyAll() {pthread_cond_broadcast(Handle());}
	
	INTRA_FORCEINLINE void Wait(pthread_mutex_t* lock) {pthread_cond_wait(Handle(), lock);}
	INTRA_FORCEINLINE void Wait(PosixMutex& lock) {Wait(lock.Handle());}

	INTRA_FORCEINLINE pthread_cond_t* Handle() {return Unsafe(&mCondVar);}

protected:
	z_D::pthread_cond_t mCondVar = INTRA_PTHREAD_COND_INITIALIZER;
};

// Cond timed wait with monotonic clock is not portable, move it into separate implementation
struct PosixCondVarMonotonic: PosixCondVar
{
	PosixCondVarMonotonic()
	{
#if !defined(INTRAZ_D_HAS_pthread_cond_timedwait_monotonic_np) && !defined(INTRAZ_D_HAS_pthread_cond_timedwait_relative_np)
		z_D::pthread_condattr_t attr;
		z_D::pthread_condattr_init(Unsafe(&attr));
		pthread_condattr_setclock(Unsafe(&attr), 1); // CLOCK_MONOTONIC: not supported on MacOS and requires Android 5.0+, so we use other methods on these platforms
		pthread_cond_init(Unsafe(&mCondVar), Unsafe(&attr));
#endif
	}
	using PosixCondVar::Wait;
	INTRA_FORCEINLINE bool Wait(pthread_mutex_t* lock, Timestamp timeout)
	{
		auto condwait = &z_D::pthread_cond_timedwait;
#ifdef INTRAZD_HAS_pthread_cond_timedwait_monotonic_np
		condwait = &z_D::pthread_cond_timedwait_monotonic_np;
#elif defined(__APPLE__) || defined(_WIN32)
		condwait = &z_D::pthread_cond_timedwait_relative_np;
#endif
		const auto ts = timeout.MonotonicTime.SinceEpoch.To<z_D::timespec>();
		return condwait(Handle(), lock, &ts) == 0;
	}
	INTRA_FORCEINLINE bool Wait(pthread_mutex_t* lock, TimeDelta timeout) {return Wait(lock, Timestamp::Now() + timeout);}
	INTRA_FORCEINLINE bool Wait(PosixMutex& lock, Timestamp timeout) {return Wait(lock.Handle(), timeout);}
	INTRA_FORCEINLINE bool Wait(PosixMutex& lock, TimeDelta timeout) {return Wait(lock.Handle(), timeout);}
};
#endif

#ifdef _WIN32
#ifdef _MSC_VER
inline void SetWinThreadDebugName(z_D::HANDLE thread, StringView name)
{
	// Set name of thread that can be seen in MSVC debugger.
	const uint32 id = z_D::GetThreadId(thread);

	char buf[128];
	name = Take(name, 127);
	z_D::memcpy(buf, name.data(), name.size());
	buf[name.size()] = '\0';
	struct {uint32 dwType; const char* szName; uint32 dwThreadID; uint32 dwFlags;}
	info = {0x1000, buf, id, 0};

#pragma warning(disable: 6320 6322 4201)
	__try {z_D::RaiseException(0x406D1388, 0, sizeof(info) / sizeof(void*), Unsafe(&info));}
	__except(1) {}
}
#endif
}

class WinThread
{
public:
	WinThread() = default;
	WinThread(const WinThread& rhs) = delete;
	WinThread& operator=(const WinThread&) = delete;
	WinThread(WinThread&& rhs) noexcept = default;
	WinThread& operator=(WinThread&&) = default;

	INTRA_FORCEINLINE explicit WinThread(z_D::HANDLE threadHandle): mThread(threadHandle) {}

	explicit WinThread(PCallable<void()> threadFunc, size_t reservedStackSize = 0)
	{
		enum {StackSizeParamIsAReservation = 0x00010000};
		const auto threadInt = z_D::_beginthreadex(nullptr, uint32(reservedStackSize),
			threadProc, threadFunc.release(), StackSizeParamIsAReservation, nullptr);
		if(threadInt) mThread = threadInt;
	}

	~WinThread() {INTRA_PRECONDITION(!IsJoinable());}

	[[nodiscard]] INTRA_FORCEINLINE bool IsJoinable() const {return !mThread.IsNull();}

	/// Block running of current thread until this thread finishes
	INTRA_FORCEINLINE void Join() {z_D::WaitForSingleObject(Handle(), 0xFFFFFFFF); mThread = {};}

	/// This thread continues running independently of this object
	INTRA_FORCEINLINE void Detach() {z_D::CloseHandle(Handle()); mThread = {};}

	[[nodiscard]] INTRA_FORCEINLINE z_D::HANDLE Handle() const {return z_D::HANDLE(mThread.Id);}

	// Schedule to call a callback on this thread when it is in alertable state
	template<CCallable F> void ScheduleCallback(F&& callback)
	{
		if constexpr(sizeof(callback) <= sizeof(ULONG_PTR))
		{
			z_D::QueueUserAPC([](ULONG_PTR callbackStorage) {
				reinterpret_cast<F&>(callbackStorage)();
			}, Handle(), reinterpret_cast<ULONG_PTR&>(callback));
		}
		else if constexpr(CSame<TRemoveReference<F>, PCallable<void()>> && !CLValueReference<F>)
		{
			z_D::QueueUserAPC([](ULONG_PTR callbackPtr) {
				TRemoveReference<F>(reinterpret_cast<ICallaback<void()>*>(callbackPtr))();
			}, Handle(), ULONG_PTR(callback.release()));
		}
		else ScheduleCallback(ToPolymorphic(INTRA_FWD(callback)));
	}

private:
	HandleMovableWrapper<index_t, -1> mThread;

	static uint32 INTRA_FORCEALIGN_ARG INTRA_WINAPI threadProc(void* lpParam)
	{
		INTRA_FINALLY {
			z_D::_endthreadex(0);
		};

		const auto threadFunc = Unique(static_cast<ICallable<void()>*>(lpParam));
		(*threadFunc)();
		return 0;
	}
};

template<bool Recursive> struct WinCriticalSection
{
	WinCriticalSection(const WinCriticalSection&) = delete;
	WinCriticalSection& operator=(const WinCriticalSection&) = delete;

	INTRA_FORCEINLINE WinCriticalSection() {z_D::InitializeCriticalSection(Handle());}
	INTRA_FORCEINLINE ~WinCriticalSection() {z_D::DeleteCriticalSection(Handle());}
	INTRA_FORCEINLINE void Lock()
	{
		z_D::EnterCriticalSection(Handle());
		if constexpr(!Recursive) INTRA_DEBUG_ASSERT(mCriticalSection.RecursionCount == 1);
	}
	INTRA_FORCEINLINE bool TryLock()
	{
		const bool res = z_D::TryEnterCriticalSection(Handle()) != 0;
		if constexpr(!Recursive) INTRA_DEBUG_ASSERT(mCriticalSection.RecursionCount == 1);
		return res;
	}
	INTRA_FORCEINLINE void Unlock() {z_D::LeaveCriticalSection(Handle());}

	[[nodiscard]] INTRA_FORCEINLINE _RTL_CRITICAL_SECTION* Handle() {return Unsafe(&mCriticalSection);}

private:
	z_D::CRITICAL_SECTION mCriticalSection;
};

#if INTRA_BUILD_FOR_WINDOWS_XP
#ifdef _MSC_VER
#pragma comment(lib, "ntdll.lib")
#endif
struct WinSRWLockXP
{
	WinSRWLockXP(const WinSRWLockXP&) = delete;
	WinSRWLockXP& operator=(const WinSRWLockXP&) = delete;

	INTRA_FORCEINLINE WinSRWLockXP() {z_D::RtlInitializeResource(&mSRWLock);}
	INTRA_FORCEINLINE ~WinSRWLockXP() {z_D::RtlDeleteResource(&mSRWLock);}
	INTRA_FORCEINLINE void Lock() {z_D::RtlAcquireResourceExclusive(&mSRWLock, true);}
	[[nodiscard]] INTRA_FORCEINLINE bool TryLock() {return z_D::RtlAcquireResourceExclusive(&mSRWLock, false) != 0;}
	INTRA_FORCEINLINE void Unlock() {z_D::RtlReleaseResource(&mSRWLock);}
	INTRA_FORCEINLINE void LockShared() {z_D::RtlAcquireResourceShared(&mSRWLock, true);}
	INTRA_FORCEINLINE bool TryLockShared() {return z_D::RtlAcquireResourceShared(&mSRWLock, false) != 0;}
	INTRA_FORCEINLINE void UnlockShared() {Unlock();}
private:
	z_D::_RTL_RWLOCK mSRWLock;
};
#endif

struct WinSRWLock
{
	WinSRWLock(const WinSRWLock&) = delete;
	WinSRWLock& operator=(const WinSRWLock&) = delete;

	WinSRWLock() = default;
	INTRA_FORCEINLINE void Lock() {z_D::AcquireSRWLockExclusive(Handle());}
	INTRA_FORCEINLINE bool TryLock() {return z_D::TryAcquireSRWLockExclusive(Handle()) != 0;}
	INTRA_FORCEINLINE void Unlock() {z_D::ReleaseSRWLockExclusive(Handle());}
	INTRA_FORCEINLINE void LockShared() {z_D::AcquireSRWLockShared(Handle());}
	INTRA_FORCEINLINE bool TryLockShared() {return z_D::TryAcquireSRWLockShared(Handle()) != 0;}
	INTRA_FORCEINLINE void UnlockShared() {z_D::ReleaseSRWLockShared(Handle());}

	INTRA_FORCEINLINE _RTL_SRWLOCK* Handle() {return Unsafe(&mSRWLock);}

private:
	void* mSRWLock = nullptr;
};

struct WinCondVar
{
	WinCondVar(const WinCondVar&) = delete;
	WinCondVar& operator=(const WinCondVar&) = delete;

	WinCondVar() = default;
	bool Wait(_RTL_CRITICAL_SECTION* lock, TimeDelta timeout)
	{
		return z_D::SleepConditionVariableCS(Handle(), lock,
			uint32(Min(Max(timeout.IntMilliseconds(), 0), MaxValueOf<uint32>))) != 0;
	}
	bool Wait(_RTL_SRWLOCK* lock, TimeDelta timeout)
	{
		return z_D::SleepConditionVariableSRW(Handle(), lock,
			uint32(Min(Max(timeout.IntMilliseconds(), 0), MaxValueOf<uint32>)), 0) != 0;
	}
	void NotifyOne() {z_D::WakeConditionVariable(Handle());}
	void NotifyAll() {z_D::WakeAllConditionVariable(Handle());}

	bool Wait(WinCriticalSection& lock, TimeDelta timeout) {return Wait(lock.Handle(), timeout);}
	bool Wait(WinSRWLock& lock, TimeDelta timeout) {return Wait(lock.Handle(), timeout);}
	void Wait(_RTL_CRITICAL_SECTION* lock) {if(!Wait(lock, 0xFFFFFFFF)) INTRA_FATAL_ERROR("Wait failed!");}
	void Wait(_RTL_SRWLOCK* lock) {if(!Wait(lock, {0xFFFFFFFF})) INTRA_FATAL_ERROR("Wait failed!");}
	void Wait(WinCriticalSection& lock) {Wait(lock.Handle());}
	void Wait(WinSRWLock& lock) {Wait(lock.Handle());}

	bool Wait(auto& lock, Timestamp timeout) {return Wait(lock, timeout.MonotonicTime - MonotonicTimestamp::Now());}
	bool Wait(auto& lock, Timestamp timeout, CCallableWithSignature<bool()> auto&& stopWaiting)
	{
		while(!stopWaiting())
			if(!Wait(lock, timeout))
				return stopWaiting();
		return true;
	}
	INTRA_FORCEINLINE void Wait(auto& lock, CCallableWithSignature<bool()> auto&& stopWaiting) {while(!stopWaiting()) Wait(lock);}

	[[nodiscard]] INTRA_FORCEINLINE _RTL_CONDITION_VARIABLE* Handle() {return Unsafe(&mCondVar);}

private:
	void* mCondVar = nullptr;
};
#endif

class DummyLockable
{
public:
	void Lock() {}
	bool TryLock() {return true;}
	void Unlock() {}
};

struct DummyCondVar
{
	template<typename T> void Wait(T&) {}
	template<typename T> bool Wait(T&, Timestamp) {return true;}
	template<typename T> bool Wait(T&, TimeDelta) {return true;}
	void NotifyOne() {}
	void NotifyAll() {}
};

#ifdef _WIN32
using BaseThread = WinThread;
#if INTRA_BUILD_FOR_WINDOWS_XP
using Mutex = WinCriticalSection<false>;
using RWMutex = WinSRWLockXP;
// NOTE: CondVar is unavailable in XP builds
//using CondVar = DummyCondVar;
#else
using Mutex = WinSRWLock;
using RWMutex = WinSRWLock;
using CondVar = WinCondVar;
#endif
#else
using BaseThread = PosixThread;
using Mutex = PosixMutex;
using RWMutex = PosixRWLock;
using CondVar = PosixCondVar;
#endif

class StopToken;
class StopSource
{
public:
	StopSource(): mImpl(Shared<Impl>::New()) {}
	[[nodiscard]] StopToken GetToken() const;
	[[nodiscard]] bool StopRequested() const {return mImpl->Flag.Get();}
	bool RequestStop() {return !mImpl->Flag.GetSet();}
private:
	friend class StopToken;
	explicit StopSource(int) {}
	struct Impl
	{
		Atomic<bool> Flag{};
	};
	Shared<Impl> mImpl;
};

class StopToken
{
public:
	[[nodiscard]] bool StopRequested() const {return mStopSource.StopRequested();}
private:
	StopSource mStopSource{0};
	friend StopSource;
};

inline StopToken StopSource::GetToken() const
{
	StopToken res;
	res.mStopSource = *this;
	return res;
}

// Mimics std::jthread from C++20
class Thread: StopSource, public BaseThread
{
public:
	Thread() = default;
	Thread(Thread&&) = default;

	explicit Thread(PCallable<void()> entryPoint): BaseThread(INTRA_MOVE(entryPoint)) {}
	template<CCallable<StopToken> Func> explicit Thread(Func&& entryPoint):
		BaseThread(BindMut(INTRA_FWD(entryPoint), GetToken())) {}

	~Thread()
	{
		if(!IsJoinable()) return;
		RequestStop();
		Join();
	}

	Thread& operator=(Thread&&) = default;
};

// Lock scope guard
template<typename T> class Lock
{
	T* mLockable;
public:
	explicit Lock(T& lockable): mLockable(&lockable) {lockable.lock();}
	~Lock() {if(mLockable) mLockable->unlock();}
	void Unlock() {mLockable->unlock(); mLockable = nullptr;}
	constexpr explicit operator bool() const noexcept {return true;}

	Lock(const Lock&) = delete;
	Lock& operator=(const Lock&) = delete;
};

/// Useful to create synchronized code blocks:
/*
INTRA_SYNCHRONIZED(mutex)
{ //mutex acquired

} //mutex releases
*/
#define INTRA_SYNCHRONIZED(lockable) if(auto INTRA_CONCATENATE_TOKENS(locker__, __LINE__) = ::Intra::Lock(lockable))

} INTRA_END
