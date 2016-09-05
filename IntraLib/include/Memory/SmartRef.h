#pragma once

#include "Threading/Atomic.h"

namespace Intra { namespace Memory {

template<typename T> struct IntrusiveRefCounted
{
	forceinline IntrusiveRefCounted(): refs(0) {}
	forceinline void AddRef() {++refs;}
	forceinline void Release() {INTRA_ASSERT(refs>0); if(--refs==0) delete (T*)this;}
	forceinline uint GetRefCount() {return refs;}

private:
	IntrusiveRefCounted(const IntrusiveRefCounted&) {refs=0;}
	IntrusiveRefCounted& operator=(const IntrusiveRefCounted&) = delete;

	Atomic<uint> refs;
};

template<typename T> struct IntrusiveReference
{
	forceinline IntrusiveReference(T* b=null) {ptr=b; if(ptr!=null) ptr->AddRef();}
	forceinline IntrusiveReference(const IntrusiveReference& rhs) {ptr=rhs.ptr; if(ptr!=null) ptr->AddRef();}
	forceinline IntrusiveReference(IntrusiveReference&& rhs) {ptr=rhs.ptr; rhs.ptr=null;}
	forceinline ~IntrusiveReference() {if(ptr!=null) ptr->Release();}

	IntrusiveReference& operator=(const IntrusiveReference& rhs)
	{
		if(ptr==rhs.ptr) return *this;
		if(ptr!=null) ptr->Release();
		ptr=rhs.ptr;
		if(ptr!=null) ptr->AddRef();
		return *this;
	}

	IntrusiveReference& operator=(IntrusiveReference&& rhs)
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
	forceinline bool operator==(const IntrusiveReference& rhs) const {return ptr==rhs.ptr;}
	forceinline bool operator!=(const IntrusiveReference& rhs) const {return !operator==(rhs);}

	T* ptr;
};

template<typename T> uint ToHash(const IntrusiveReference<T>& rhs) {return ToHash(rhs.ptr);}


template<typename T> struct UniqueReference
{
	forceinline UniqueReference(T* b=null) {ptr=b;}
	forceinline UniqueReference(const UniqueReference& rhs) = delete;
	forceinline UniqueReference(UniqueReference&& rhs) {ptr=rhs.ptr; rhs.ptr=null;}
	forceinline ~UniqueReference() {if(ptr!=null) delete ptr;}

	UniqueReference& operator=(const UniqueReference& rhs) = delete;

	UniqueReference& operator=(UniqueReference&& rhs)
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
