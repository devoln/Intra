#pragma once

#include "Platform/FundamentalTypes.h"
#include "Platform/Debug.h"
#include "Platform/CppFeatures.h"
#include "Platform/CppWarnings.h"

namespace Intra { namespace Memory {

INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS

template<typename T> struct Unique
{
	forceinline Unique(null_t=null): mPtr(null) {}

	//! Этот конструктор предполагает, что объект выделен как new T(...).
	//! Передавать другие указатели запрещено.
	forceinline Unique(T*&& ptrFromNew): mPtr(ptrFromNew) {}

	forceinline ~Unique() {if(mPtr!=null) delete mPtr;}

	forceinline Unique(const Unique& rhs) = delete;
	Unique& operator=(const Unique& rhs) = delete;

	forceinline Unique(Unique&& rhs):
		mPtr(rhs.mPtr) {rhs.mPtr=null;}

	template<typename... Args> forceinline static Unique New(Args&&... args)
	{return new T(Meta::Forward<Args>(args)...);}

	Unique& operator=(Unique&& rhs)
	{
		if(this == &rhs) return *this;
		delete mPtr;
		mPtr = rhs.mPtr;
		rhs.mPtr = null;
		return *this;
	}

	Unique& operator=(T* ptrFromNew)
	{
		if(mPtr == ptrFromNew) return *this;
		delete mPtr;
		mPtr = ptrFromNew;
		return *this;
	}

	forceinline Unique& operator=(null_t)
	{
		delete mPtr;
		mPtr = null;
		return *this;
	}

	forceinline T* Ptr() const {return mPtr;}

	forceinline T* Release()
	{
		T* result = mPtr;
		mPtr = null;
		return result;
	}

	forceinline T& operator*() const {INTRA_DEBUG_ASSERT(mPtr!=null); return *mPtr;}
	forceinline T* operator->() const {INTRA_DEBUG_ASSERT(mPtr!=null); return mPtr;}

	forceinline bool operator==(null_t) const {return mPtr==null;}
	forceinline bool operator!=(null_t) const {return !operator==(null);}

private:
	T* mPtr;
};

INTRA_WARNING_POP

template<typename T> forceinline Unique<T> UniqueMove(T& rhs) {return new T(Meta::Move(rhs));}

}
using Memory::Unique;
using Memory::UniqueMove;

}
