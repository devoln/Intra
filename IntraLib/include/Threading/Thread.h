#pragma once

#include "Platform/CppWarnings.h"
#include "Utils/Callback.h"

#undef Yield

namespace Intra {

INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS

class Thread
{
	struct Handle;
#if(INTRA_LIBRARY_THREADING!=INTRA_LIBRARY_THREADING_Dummy)
	Handle* handle;
#endif
public:
	typedef Utils::Delegate<void()> Func;

#if(INTRA_LIBRARY_THREADING!=INTRA_LIBRARY_THREADING_Dummy)
	Thread(null_t=null): handle(null) {}
	Thread(Thread&& rhs): handle(rhs.handle) {rhs.handle=null;}
	Thread(const Func& func): handle(null) {create_thread(func);}

	Thread& operator=(Thread&& rhs)
	{
		if(handle!=null) delete_thread();
		handle = rhs.handle;
		rhs.handle = null;
		return *this;
	}

	~Thread() {delete_thread();}
#else
	Thread(null_t=null) {}
	Thread(Thread&& rhs) {(void)rhs;}
	Thread(const Func& func) {create_thread(func);}
	Thread& operator=(Thread&& rhs) {(void)rhs; return *this;}
#endif

	void Join();
	void Detach();
	bool Joinable() const;

	static void Yield();

private:
	void create_thread(const Func& func);
	void delete_thread();


	Thread(const Thread& rhs) = delete;
	Thread& operator=(const Thread&) = delete;
};

class Mutex
{
	struct Handle;
#if(INTRA_LIBRARY_THREADING!=INTRA_LIBRARY_THREADING_Dummy)
	Handle* handle;
#endif
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
