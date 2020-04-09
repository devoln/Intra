#pragma once

#include "Intra/Core.h"
#include "Intra/Preprocessor.h"

INTRA_BEGIN
// Lock scope guard
template<typename T> class Lock
{
	T* mLockable;
public:
	INTRA_FORCEINLINE Lock(T& lockable): mLockable(&lockable) {lockable.Lock();}
	INTRA_FORCEINLINE Lock(Lock&& rhs): mLockable(rhs.mLockable) {rhs.mLockable = null;}
	INTRA_FORCEINLINE ~Lock() {mLockable->Unlock();}
	INTRA_FORCEINLINE operator bool() const noexcept {return true;}

	T& Primitive() {return *mLockable;}

private:
	Lock(const Lock&) = delete;
	Lock& operator=(const Lock&) = delete;
};

template<typename T> static INTRA_FORCEINLINE Lock<T> MakeLock(T& lockable) {return lockable;}
template<typename T> static INTRA_FORCEINLINE Lock<T> MakeLock(T* lockable) {return *lockable;}

//! Useful to create synchronized code blocks:
/*!
INTRA_SYNCHRONIZED(mutex)
{ //mutex acquired

} //mutex releases
*/
#define INTRA_SYNCHRONIZED(lockable) \
	if(auto INTRA_CONCATENATE_TOKENS(locker__, __LINE__) = ::Intra::MakeLock(lockable))
INTRA_END
