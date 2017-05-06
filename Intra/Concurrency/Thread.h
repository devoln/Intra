#pragma once

#include "Cpp/PlatformDetect.h"
#include "Cpp/Warnings.h"
#include "Cpp/Features.h"
#include "Cpp/Fundamental.h"

#include "Preprocessor/Preprocessor.h"

#include "Utils/Delegate.h"

#undef Yield


#define INTRA_LIBRARY_THREADING_Dummy 0
#define INTRA_LIBRARY_THREADING_WinAPI 1
#define INTRA_LIBRARY_THREADING_CPPLIB 2
#define INTRA_LIBRARY_THREADING_Qt 3
#define INTRA_LIBRARY_THREADING_SDL 4
#define INTRA_LIBRARY_THREADING_PThread 5

#ifndef INTRA_LIBRARY_THREADING

#if(INTRA_PLATFORM_OS == INTRA_PLATFORM_OS_Windows)
#define INTRA_LIBRARY_THREADING INTRA_LIBRARY_THREADING_WinAPI
#elif(INTRA_PLATFORM_OS == INTRA_PLATFORM_OS_Emscripten)

#ifdef __EMSCRIPTEN_PTHREADS__ 
#define INTRA_LIBRARY_THREADING INTRA_LIBRARY_THREADING_PThread
#else
#define INTRA_LIBRARY_THREADING INTRA_LIBRARY_THREADING_Dummy
#endif

#else
#define INTRA_LIBRARY_THREADING INTRA_LIBRARY_THREADING_PThread
#endif

#endif

INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS

namespace Intra { namespace Concurrency {

class Thread
{
	struct Data;
	typedef Data* Handle;
#if(INTRA_LIBRARY_THREADING!=INTRA_LIBRARY_THREADING_Dummy)
	Handle mHandle;
	mutable bool mJoined;
#endif
	uint mId;
	struct NativeData;
public:
	typedef NativeData* NativeHandle;

	typedef Utils::Delegate<void()> Func;

#if(INTRA_LIBRARY_THREADING!=INTRA_LIBRARY_THREADING_Dummy)
	Thread(null_t=null): mHandle(null), mJoined(false), mId(0) {}
	Thread(Thread&& rhs): mHandle(rhs.mHandle), mJoined(rhs.mJoined), mId(rhs.mId) {rhs.mHandle=null;}
	Thread(const Func& func): mHandle(null), mJoined(false), mId(0) {create_thread(func);}

	Thread& operator=(Thread&& rhs)
	{
		if(mHandle!=null) delete_thread();
		mHandle = rhs.mHandle;
		mJoined = rhs.mJoined;
		mId = rhs.mId;
		rhs.mHandle = null;
		return *this;
	}

	//! Деструктор ждёт завершения потока.
	~Thread() {if(mHandle!=null) delete_thread();}
#else
	Thread(null_t=null) {}
	Thread(Thread&& rhs) {(void)rhs;}
	Thread(Func func) {create_thread(Cpp::Move(func));}
	Thread& operator=(Thread&& rhs) {(void)rhs; return *this;}
#endif

	//! Блокировать выполнение текущего потока до тех пор,
	//! пока указанный поток не завершится или не истечёт указанный таймаут.
	//! @param timeoutInMs Таймаут в миллисекундах.
	//! @return Возвращает true, если в результате поток завершил своё выполнение.
	bool Join(uint timeoutInMs);

	bool Join();

	//! Отсоединить поток от объекта.
	//! Текущий объект переходит в нулевое состояние,
	//! поток продолжает выполняться независимо от него.
	void Detach();

	//! Возвращает, выполняется ли поток в данный момент.
	bool IsRunning() const;

	uint Id() const {return mId;}
	static uint CurrentId();

	//! @return Зависимый от платформы дескриптор потока:
	//! Для Windows HANDLE (созданный через _beginthreadex\CreateThread)
	//! Для остальных платформ pthread*.
	//! Полученный дескриптор нельзя использовать после Detach или уничтожения потока.
	NativeHandle GetNativeHandle() const;

	//! Отдаёт текущий квант времени ОС.
	static void Yield();

private:
	void create_thread(Func func);
	void delete_thread();


	Thread(const Thread& rhs) = delete;
	Thread& operator=(const Thread&) = delete;
};


template<typename T> class LockObject
{
	T* mLockable;
public:
	forceinline LockObject(T& lockable): mLockable(&lockable) {lockable.Lock();}
	forceinline LockObject(LockObject&& rhs): mLockable(rhs.mLockable) {rhs.mLockable = null;}
	forceinline ~LockObject() {mLockable->Unlock();}
	forceinline operator bool() const {return true;}

	LockObject(const LockObject&) = delete;
	LockObject& operator=(const LockObject&) = delete;
};


class Mutex
{
#if(INTRA_LIBRARY_THREADING!=INTRA_LIBRARY_THREADING_Dummy)
	struct Data;
	typedef Data* Handle;
	Handle mHandle;
#endif
	struct NativeData;
public:
	typedef NativeData* NativeHandle;

	Mutex(bool processPrivate=true);
	~Mutex();

	void Lock();
	void Enter() {Lock();}
	bool TryLock();
	bool TryEnter() {return TryLock();}
	void Unlock();
	void Leave() {Unlock();}

	LockObject<Mutex> Locker() {return *this;}

private:
	Mutex(const Mutex&) = delete;
	Mutex& operator=(const Mutex&) = delete;
};



#if(INTRA_LIBRARY_THREADING!=INTRA_LIBRARY_THREADING_Dummy)
#define INTRA_SYNCHRONIZED_BLOCK(mutex) if(auto INTRA_CONCATENATE_TOKENS($locker, __LINE__) = mutex.Locker())
#else
#define INTRA_SYNCHRONIZED_BLOCK(mutex)
#endif

}
using Concurrency::Thread;
using Concurrency::Mutex;

}

INTRA_WARNING_POP
