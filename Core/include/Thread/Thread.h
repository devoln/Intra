#pragma once

#include "Platform/CppWarnings.h"
#include "Utils/Delegate.h"

#undef Yield

namespace Intra {

INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS

class Thread
{
	struct Data;
	typedef Data* Handle;
#if(INTRA_LIBRARY_THREADING!=INTRA_LIBRARY_THREADING_Dummy)
	Handle mHandle;
	mutable bool mJoined;
#endif
	struct NativeData;
public:
	typedef NativeData* NativeHandle;

	typedef Utils::Delegate<void()> Func;

#if(INTRA_LIBRARY_THREADING!=INTRA_LIBRARY_THREADING_Dummy)
	Thread(null_t=null): mHandle(null), mJoined(false) {}
	Thread(Thread&& rhs): mHandle(rhs.mHandle), mJoined(rhs.mJoined) {rhs.mHandle=null;}
	Thread(const Func& func): mHandle(null), mJoined(false) {create_thread(func);}

	Thread& operator=(Thread&& rhs)
	{
		if(mHandle!=null) delete_thread();
		mHandle = rhs.mHandle;
		mJoined = rhs.mJoined;
		rhs.mHandle = null;
		return *this;
	}

	//! Деструктор ждёт завершения потока.
	~Thread() {if(mHandle!=null) delete_thread();}
#else
	Thread(null_t=null) {}
	Thread(Thread&& rhs) {(void)rhs;}
	Thread(Func func) {create_thread(Meta::Move(func));}
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

	class Locker
	{
		Mutex* mutex;
	public:
		Locker(Mutex& mut) noexcept: mutex(&mut) {mut.Lock();}
		Locker(Mutex* mut) noexcept: mutex(mut) {mut->Lock();}
		~Locker() {mutex->Unlock();}
		operator bool() const {return true;}

		Locker(const Locker&) = delete;
		Locker& operator=(const Locker&) = delete;
	};

private:
	Mutex(const Mutex&) = delete;
	Mutex& operator=(const Mutex&) = delete;
};

#if(INTRA_LIBRARY_THREADING!=INTRA_LIBRARY_THREADING_Dummy)
#define INTRA_SYNCHRONIZED_BLOCK(mutex) if( Intra::Mutex::Locker _Locker{(mutex)} )
#else
#define INTRA_SYNCHRONIZED_BLOCK(mutex)
#endif

INTRA_WARNING_POP

}
