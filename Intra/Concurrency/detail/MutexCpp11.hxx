#include "Concurrency/Mutex.h"

#include <mutex>

namespace Intra { namespace Concurrency {

Mutex::Mutex()
{
	static_assert(sizeof(mData) >= sizeof(std::mutex), "Invalid DATA_SIZE in Mutex.h!");
	new(mData) std::mutex;
}
Mutex::~Mutex() {reinterpret_cast<std::mutex*>(mData)->~mutex();}
void Mutex::Lock() {reinterpret_cast<std::mutex*>(mData)->lock();}
bool Mutex::TryLock() {return reinterpret_cast<std::mutex*>(mData)->try_lock();}
void Mutex::Unlock() {reinterpret_cast<std::mutex*>(mData)->unlock();}

RecursiveMutex::RecursiveMutex()
{
	static_assert(sizeof(mData) >= sizeof(std::recursive_mutex), "Invalid DATA_SIZE in Mutex.h!");
	new(mData) std::recursive_mutex;
}
RecursiveMutex::~RecursiveMutex() {reinterpret_cast<std::recursive_mutex*>(mData)->~recursive_mutex();}
void RecursiveMutex::Lock() {reinterpret_cast<std::recursive_mutex*>(mData)->lock();}
bool RecursiveMutex::TryLock() {return reinterpret_cast<std::recursive_mutex*>(mData)->try_lock();}
void RecursiveMutex::Unlock() {reinterpret_cast<std::recursive_mutex*>(mData)->unlock();}

}}
