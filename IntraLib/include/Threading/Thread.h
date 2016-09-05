#pragma once

#include "Utils/Callback.h"

#ifdef Yield
#undef Yield
#endif

namespace Intra {

class Thread
{
	struct Handle;
public:
	typedef Utils::FixedDelegate<void()> Func;

	Thread(null_t=null): handle(null) {}
	Thread(Thread&& rhs): handle(rhs.handle) {rhs.handle=null;}
	Thread(const Func& func) {create_thread(func);}
	~Thread() {delete_thread();}

	Thread& operator=(Thread&& rhs)
	{
		if(handle!=null) delete_thread();
		handle = rhs.handle;
		rhs.handle = null;
		return *this;
	}

	void Join();
	void Detach();
	bool Joinable() const;

	static void Yield();

private:
	void create_thread(const Func& func);
	void delete_thread();

	Handle* handle;

	Thread(const Thread& rhs) = delete;
	Thread& operator=(const Thread&) = delete;
};

class Mutex
{
public:
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
	struct Handle;
	Handle* handle;

	Mutex(const Mutex&) = delete;
	Mutex& operator=(const Mutex&) = delete;
};

#if(INTRA_LIBRARY_THREADING!=INTRA_LIBRARY_THREADING_Dummy)
#define INTRA_SYNCHRONIZED_BLOCK(mutex) if( Intra::Mutex::Locker _Locker{(mutex)} )
#else
#define INTRA_SYNCHRONIZED_BLOCK(mutex)
#endif

}
