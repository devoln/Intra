#include "Concurrency/Mutex.h"

#include <mutex>

namespace Intra { namespace Concurrency {

struct Mutex::Data
{
	std::mutex mut;
};

Mutex::Mutex() {mHandle = new Data;}
Mutex::~Mutex() {delete mHandle;}
void Mutex::Lock() {mHandle->mut.lock();}
bool Mutex::TryLock() {return mHandle->mut.try_lock();}
void Mutex::Unlock() {mHandle->mut.unlock();}

}}
