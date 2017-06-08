#pragma once

#include "Cpp/Features.h"
#include "Cpp/Warnings.h"

#include "Preprocessor/Operations.h"

INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS

namespace Intra { namespace Concurrency {

template<typename T> class Lock
{
	T* mLockable;
public:
	forceinline Lock(T& lockable): mLockable(&lockable) {lockable.Lock();}
	forceinline Lock(Lock&& rhs): mLockable(rhs.mLockable) {rhs.mLockable = null;}
	forceinline ~Lock() {mLockable->Unlock();}
	forceinline operator bool() const {return true;}

private:
	Lock(const Lock&) = delete;
	Lock& operator=(const Lock&) = delete;
};

template<typename T> static forceinline Lock<T> MakeLock(T& lockable) {return lockable;}

#define INTRA_SYNCHRONIZED_BLOCK(lockable) \
	if(auto INTRA_CONCATENATE_TOKENS(locker__, __LINE__) = ::Intra::Concurrency::MakeLock(lockable))

}}

INTRA_WARNING_POP
