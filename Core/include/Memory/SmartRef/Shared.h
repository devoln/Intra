#pragma once

#include "Platform/FundamentalTypes.h"
#include "Platform/Debug.h"
#include "Platform/CppFeatures.h"
#include "Platform/CppWarnings.h"

#include "Thread/Atomic.h"

namespace Intra { namespace Memory {

INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS

template<typename T> struct IntrusiveRefCounted
{
	forceinline IntrusiveRefCounted(): mRefs(0) {}
	
	forceinline void AddRef() {++mRefs;}
	
	forceinline void Release()
	{
		INTRA_ASSERT(mRefs>0);
		if(--mRefs==0) delete static_cast<T*>(this);
	}

	forceinline uint GetRefCount() {return mRefs;}

private:
	IntrusiveRefCounted(const IntrusiveRefCounted&) {mRefs = 0;}
	IntrusiveRefCounted& operator=(const IntrusiveRefCounted&) = delete;

	Atomic<uint> mRefs;
};

template<typename T> struct IntrusiveRef
{
	forceinline IntrusiveRef(T* b=null): mPtr(b) {if(mPtr!=null) mPtr->AddRef();}
	forceinline IntrusiveRef(const IntrusiveRef& rhs): mPtr(rhs.mPtr) {if(mPtr!=null) mPtr->AddRef();}
	forceinline IntrusiveRef(IntrusiveRef&& rhs): mPtr(rhs.mPtr) {rhs.mPtr = null;}
	forceinline ~IntrusiveRef() {if(mPtr!=null) mPtr->Release();}

	IntrusiveRef& operator=(const IntrusiveRef& rhs)
	{
		if(mPtr==rhs.mPtr) return *this;
		if(mPtr!=null) mPtr->Release();
		mPtr = rhs.mPtr;
		if(mPtr!=null) mPtr->AddRef();
		return *this;
	}

	IntrusiveRef& operator=(IntrusiveRef&& rhs)
	{
		if(mPtr==rhs.mPtr) return *this;
		if(mPtr!=null) mPtr->Release();
		mPtr = rhs.mPtr;
		rhs.mPtr = null;
		return *this;
	}

	forceinline size_t use_count() const {return mPtr!=null? mPtr->GetRefCount(): 0;}
	forceinline bool unique() const {return use_count()==1;}

	forceinline T& operator*() const {INTRA_ASSERT(mPtr!=null); return *mPtr;}
	forceinline T* operator->() const {INTRA_ASSERT(mPtr!=null); return mPtr;}

	forceinline bool operator==(null_t) const {return mPtr==null;}
	forceinline bool operator!=(null_t) const {return !operator==(null);}
	forceinline bool operator==(const IntrusiveRef& rhs) const {return mPtr==rhs.mPtr;}
	forceinline bool operator!=(const IntrusiveRef& rhs) const {return !operator==(rhs);}

	uint ToHash() const {return ToHash(mPtr);}

	T* mPtr;
};

INTRA_WARNING_POP

}}
