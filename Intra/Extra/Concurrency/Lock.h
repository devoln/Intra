#pragma once

#include "Core/Core.h"
#include "Core/Preprocessor.h"

INTRA_BEGIN
// Lock scope guard
template<typename T> class Lock
{
	T* mLockable;
public:
	forceinline Lock(T& lockable): mLockable(&lockable) {lockable.Lock();}
	forceinline Lock(Lock&& rhs): mLockable(rhs.mLockable) {rhs.mLockable = null;}
	forceinline ~Lock() {mLockable->Unlock();}
	forceinline operator bool() const noexcept {return true;}

	T& Primitive() {return *mLockable;}

private:
	Lock(const Lock&) = delete;
	Lock& operator=(const Lock&) = delete;
};

template<typename T> static forceinline Lock<T> MakeLock(T& lockable) {return lockable;}
template<typename T> static forceinline Lock<T> MakeLock(T* lockable) {return *lockable;}

//! Useful to create synchronized code blocks:
/*!
INTRA_SYNCHRONIZED(mutex)
{ //mutex acquired

} //mutex releases
*/
#define INTRA_SYNCHRONIZED(lockable) \
	if(auto INTRA_CONCATENATE_TOKENS(locker__, __LINE__) = ::Intra::MakeLock(lockable))
INTRA_END
