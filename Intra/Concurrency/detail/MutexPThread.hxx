#include "Concurrency/Mutex.h"

#include <pthread.h>

#ifdef _MSC_VER
#pragma comment(lib, "pthread.lib")
#endif

namespace Intra { namespace Concurrency {

Mutex::Mutex(): mHandle(null)
{
	pthread_mutex_t* mutex = new pthread_mutex_t;
	pthread_mutexattr_t attr;
	pthread_mutexattr_init(&attr);
	pthread_mutex_init(mutex, &attr);
	mHandle = Handle(mutex);
}

Mutex::~Mutex()
{
	pthread_mutex_destroy(reinterpret_cast<pthread_mutex_t*>(mHandle));
	delete reinterpret_cast<pthread_mutex_t*>(mHandle);
}

void Mutex::Lock() {pthread_mutex_lock(reinterpret_cast<pthread_mutex_t*>(mHandle));}
bool Mutex::TryLock() {return pthread_mutex_trylock(reinterpret_cast<pthread_mutex_t*>(mHandle))!=0;}
void Mutex::Unlock() {pthread_mutex_unlock(reinterpret_cast<pthread_mutex_t*>(mHandle));}

}}
