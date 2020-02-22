#include "Concurrency/Mutex.h"

#include <mutex>

INTRA_BEGIN
Mutex::Mutex()
{
	static_assert(sizeof(mData) >= sizeof(std::mutex), "Invalid DATA_SIZE in Mutex.h!");
	new(mData) std::mutex;
}
Mutex::~Mutex() {reinterpret_cast<std::mutex*>(mData)->~mutex();}
void Mutex::Lock() {reinterpret_cast<std::mutex*>(mData)->lock();}
bool Mutex::TryLock() {return reinterpret_cast<std::mutex*>(mData)->try_lock();}
void Mutex::Unlock() {reinterpret_cast<std::mutex*>(mData)->unlock();}
INTRA_END
