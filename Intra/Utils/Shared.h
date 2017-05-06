#pragma once

#include "Cpp/Fundamental.h"
#include "Cpp/Features.h"
#include "Cpp/Warnings.h"

#include "Utils/Debug.h"

#ifndef INTRA_UTILS_NO_CONCURRENCY
#include "Concurrency/Atomic.h"
#endif

namespace Intra { namespace Memory {

INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS

//! Более лёгкий аналог std::shared_ptr.
//! Он не принимает сырые указатели, поэтому не подвержен ошибке наличия двух счётчиков для одного объекта.
//! Выделяет и освобождает память сам через new и не хранит deleter.
//! Не поддерживает слабые ссылки.
//! Также он не может использоваться с incomplete типами.
template<typename T> class Shared
{
	struct Data
	{
		template<typename... Args> forceinline Data(Args&&... args):
			Value(Cpp::Forward<Args>(args)...), RefCount(1) {}

		forceinline void Release()
		{
			INTRA_DEBUG_ASSERT(RefCount > 0);
			if(--RefCount == 0) delete this;
		}
		
		T Value;
#ifndef INTRA_UTILS_NO_CONCURRENCY
		Atomic<uint> RefCount;
#endif

		Data(const Data&) = delete;
		Data& operator=(const Data&) = delete;
	};
	forceinline Shared(Data* data): mData(data) {}
public:
	forceinline Shared(null_t=null): mData(null) {}
	forceinline Shared(const Shared& rhs): mData(rhs.mData) {if(mData!=null) ++mData->RefCount;}
	forceinline Shared(Shared&& rhs): mData(rhs.mData) {rhs.mData = null;}
	forceinline ~Shared() {if(mData!=null) mData->Release();}

	Shared& operator=(const Shared& rhs)
	{
		if(mData==rhs.mData) return *this;
		if(mData!=null) mData->Release();
		mData = rhs.mData;
		if(mData!=null) ++mData->RefCount;
		return *this;
	}

	Shared& operator=(Shared&& rhs)
	{
		if(mData==rhs.mData) return *this;
		if(mData!=null) mData->Release();
		mData = rhs.mData;
		rhs.mData = null;
		return *this;
	}

	forceinline Shared& operator=(null_t)
	{
		if(mData==null) return *this;
		mData->Release();
		mData = null;
		return *this;
	}

	template<typename... Args> forceinline static Shared New(Args&&... args)
	{return new Data(Cpp::Forward<Args>(args)...);}

	forceinline uint use_count() const
	{
		if(mData == null) return 0;
		return mData->RefCount;
	}

	forceinline T& operator*() const {INTRA_DEBUG_ASSERT(mData!=null); return mData->Value;}
	forceinline T* operator->() const {INTRA_DEBUG_ASSERT(mData!=null); return &mData->Value;}

	forceinline bool operator==(null_t) const {return mData==null;}
	forceinline bool operator!=(null_t) const {return !operator==(null);}
	forceinline bool operator==(const Shared& rhs) const {return mData==rhs.mData;}
	forceinline bool operator!=(const Shared& rhs) const {return !operator==(rhs);}

private:
	Data* mData;
};

template<typename T> forceinline Shared<Meta::RemoveReference<T>> SharedMove(T&& rhs)
{return Shared<Meta::RemoveReference<T>>::New(Cpp::Move(rhs));}

INTRA_WARNING_POP

}
using Memory::Shared;
using Memory::SharedMove;

}
