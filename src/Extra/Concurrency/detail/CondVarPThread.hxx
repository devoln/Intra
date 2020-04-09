#include "Extra/Concurrency/CondVar.h"

#include <pthread.h>

#ifdef _MSC_VER
#pragma comment(lib, "pthread.lib")
#endif

INTRA_BEGIN
SeparateCondVar::SeparateCondVar()
{
	static_assert(sizeof(mData) >= sizeof(pthread_cond_t), "Invalid DATA_SIZE in CondVar.h!");
	pthread_cond_init(reinterpret_cast<pthread_cond_t*>(mData), null);
}

SeparateCondVar::~SeparateCondVar() {pthread_cond_destroy(reinterpret_cast<pthread_cond_t*>(mData));}

bool SeparateCondVar::wait(Mutex& lock)
{return pthread_cond_wait(reinterpret_cast<pthread_cond_t*>(mData), reinterpret_cast<pthread_mutex_t*>(lock.mData)) == 0;}

bool SeparateCondVar::waitUntil(Mutex& mutex, uint64 absTimeMs)
{
	timespec absTime = {time_t(absTimeMs / 1000), long((absTimeMs % 1000) * 1000000)};
	return pthread_cond_timedwait(reinterpret_cast<pthread_cond_t*>(mData), reinterpret_cast<pthread_mutex_t*>(mutex.mData), &absTime) == 0;
}

void SeparateCondVar::Notify() {pthread_cond_signal(reinterpret_cast<pthread_cond_t*>(mData));}
void SeparateCondVar::NotifyAll() {pthread_cond_broadcast(reinterpret_cast<pthread_cond_t*>(mData));}
INTRA_END
