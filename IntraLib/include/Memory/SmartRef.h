#pragma once

#include "Threading/Atomic.h"

namespace Intra { namespace Memory {

template<typename T> struct IntrusiveRefCounted
{
	forceinline IntrusiveRefCounted(): refs(0) {}
	
	forceinline void AddRef() {++refs;}
	
	forceinline void Release()
	{
		INTRA_ASSERT(refs>0);
		if(--refs==0) delete static_cast<T*>(this);
	}

	forceinline uint GetRefCount() {return refs;}

private:
	IntrusiveRefCounted(const IntrusiveRefCounted&) {refs=0;}
	IntrusiveRefCounted& operator=(const IntrusiveRefCounted&) = delete;

	Atomic<uint> refs;
};

template<typename T> struct IntrusiveRef
{
	forceinline IntrusiveRef(T* b=null) {ptr=b; if(ptr!=null) ptr->AddRef();}
	forceinline IntrusiveRef(const IntrusiveRef& rhs) {ptr=rhs.ptr; if(ptr!=null) ptr->AddRef();}
	forceinline IntrusiveRef(IntrusiveRef&& rhs) {ptr=rhs.ptr; rhs.ptr=null;}
	forceinline ~IntrusiveRef() {if(ptr!=null) ptr->Release();}

	IntrusiveRef& operator=(const IntrusiveRef& rhs)
	{
		if(ptr==rhs.ptr) return *this;
		if(ptr!=null) ptr->Release();
		ptr=rhs.ptr;
		if(ptr!=null) ptr->AddRef();
		return *this;
	}

	IntrusiveRef& operator=(IntrusiveRef&& rhs)
	{
		if(ptr==rhs.ptr) return *this;
		if(ptr!=null) ptr->Release();
		ptr=rhs.ptr;
		rhs.ptr=null;
		return *this;
	}

	forceinline size_t use_count() const {return ptr!=null? ptr->GetRefCount(): 0;}
	forceinline bool unique() const {return use_count()==1;}

	forceinline T& operator*() const {INTRA_ASSERT(ptr!=null); return *ptr;}
	forceinline T* operator->() const {INTRA_ASSERT(ptr!=null); return ptr;}

	forceinline bool operator==(null_t) const {return ptr==null;}
	forceinline bool operator!=(null_t) const {return !operator==(null);}
	forceinline bool operator==(const IntrusiveRef& rhs) const {return ptr==rhs.ptr;}
	forceinline bool operator!=(const IntrusiveRef& rhs) const {return !operator==(rhs);}

	T* ptr;
};

template<typename T> uint ToHash(const IntrusiveRef<T>& rhs) {return ToHash(rhs.ptr);}


template<typename T> struct UniqueRef
{
	forceinline UniqueRef(T* b=null): ptr(b) {}
	forceinline ~UniqueRef() {if(ptr!=null) delete ptr;}

	forceinline UniqueRef(const UniqueRef& rhs) = delete;
	UniqueRef& operator=(const UniqueRef& rhs) = delete;

	forceinline UniqueRef(UniqueRef&& rhs):
		ptr(rhs.ptr) {rhs.ptr=null;}

	UniqueRef& operator=(UniqueRef&& rhs)
	{
		if(ptr!=null) delete ptr;
		ptr = rhs.ptr;
		rhs.ptr = null;
		return *this;
	}

	forceinline T& operator*() const {INTRA_ASSERT(ptr!=null); return *ptr;}
	forceinline T* operator->() const {INTRA_ASSERT(ptr!=null); return ptr;}

	forceinline bool operator==(null_t) const {return ptr==null;}
	forceinline bool operator!=(null_t) const {return !operator==(null);}

	T* ptr;
};

}}
