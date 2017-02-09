#pragma once

#include "Platform/FundamentalTypes.h"
#include "Platform/Debug.h"
#include "Platform/CppFeatures.h"
#include "Platform/CppWarnings.h"

namespace Intra { namespace Memory {

INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS

template<typename T> struct UniqueRef
{
	forceinline UniqueRef(T* b=null): mPtr(b) {}
	forceinline ~UniqueRef() {if(mPtr!=null) delete mPtr;}

	forceinline UniqueRef(const UniqueRef& rhs) = delete;
	UniqueRef& operator=(const UniqueRef& rhs) = delete;

	forceinline UniqueRef(UniqueRef&& rhs):
		mPtr(rhs.mPtr) {rhs.mPtr=null;}

	UniqueRef& operator=(UniqueRef&& rhs)
	{
		if(mPtr!=null) delete mPtr;
		mPtr = rhs.mPtr;
		rhs.mPtr = null;
		return *this;
	}

	T* Ptr() const {return mPtr;}

	forceinline T& operator*() const {INTRA_ASSERT(mPtr!=null); return *mPtr;}
	forceinline T* operator->() const {INTRA_ASSERT(mPtr!=null); return mPtr;}

	forceinline bool operator==(null_t) const {return mPtr==null;}
	forceinline bool operator!=(null_t) const {return !operator==(null);}

private:
	T* mPtr;
};

INTRA_WARNING_POP

}}
