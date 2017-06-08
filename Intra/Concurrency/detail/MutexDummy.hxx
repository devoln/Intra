#include "Concurrency/Mutex.h"

namespace Intra { namespace Concurrency {

Mutex::Mutex() {}
Mutex::~Mutex() {}
void Mutex::Lock() {}
bool Mutex::TryLock() {return true;}
void Mutex::Unlock() {}

}}
