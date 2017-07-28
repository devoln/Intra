#pragma once

#include "Cpp/Fundamental.h"
#include "Cpp/Features.h"
#include "Cpp/Warnings.h"
#include "Cpp/PlatformDetect.h"

#include "Utils/Debug.h"

#ifndef INTRA_NO_CONCURRENCY
#include "Concurrency/Atomic.h"
#endif

namespace Intra { namespace Utils {

INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS

//! Более лёгкий аналог std::shared_ptr.
//! Он не принимает сырые указатели, поэтому не подвержен ошибке наличия двух счётчиков для одного объекта.
//! Выделяет и освобождает память сам через new и не хранит deleter.
//! Не поддерживает слабые ссылки.
//! Также он не может использоваться с incomplete типами.
template<typename T> class Shared
{
	template<typename U> friend class SharedClass;
	struct Data
	{
		template<typename... Args> forceinline Data(Args&&... args):
			Value(Cpp::Forward<Args>(args)...), RefCount(1) {}

		forceinline void Release()
		{
			INTRA_DEBUG_ASSERT(GetRC() != 0);
			if(DecRef()) delete this;
		}
		
		T Value;
#if(!defined(INTRA_NO_CONCURRENCY) && INTRA_LIBRARY_ATOMIC != INTRA_LIBRARY_ATOMIC_None)
		AtomicInt RefCount;
		forceinline uint IncRef() {return uint(RefCount.GetIncrementRelaxed());}
		forceinline uint GetRC() {return uint(RefCount.GetRelaxed());}
		forceinline bool DecRef() {return RefCount.DecrementAcquireRelease() == 0;}
#else
		forceinline uint IncRef() {return RefCount++;}
		forceinline uint GetRC() {return RefCount;}
		forceinline bool DecRef() {return --RefCount == 0;}
		uint RefCount;
#endif

		Data(const Data&) = delete;
		Data& operator=(const Data&) = delete;
	};
	constexpr forceinline Shared(Data* data): mData(data) {}
public:
	constexpr forceinline Shared(null_t=null): mData(null) {}

	forceinline Shared(const Shared& rhs): mData(rhs.mData)
	{
		if(mData != null) mData->IncRef();
	}

	template<typename U, typename = Meta::EnableIf<
		Meta::IsInherited<U, T>::_// && Meta::HasVirtualDestructor<T>::_
	>> forceinline Shared(const Shared<U>& rhs): mData(rhs.mData)
	{
		if(mData != null) mData->IncRef();
	}

	forceinline Shared(Shared&& rhs): mData(rhs.mData) {rhs.mData = null;}
	
	template<typename U, typename = Meta::EnableIf<
		Meta::IsInherited<U, T>::_// && Meta::HasVirtualDestructor<T>::_
	>> forceinline Shared(Shared<U>&& rhs): mData(rhs.mData) {rhs.mData = null;}

	forceinline ~Shared() {if(mData != null) mData->Release();}

	Shared& operator=(const Shared& rhs)
	{
		if(mData == rhs.mData) return *this;
		Shared temp(rhs);
		Cpp::Swap(mData, temp.mData);
		return *this;
	}

	forceinline Shared& operator=(Shared&& rhs)
	{
		Cpp::Swap(mData, rhs.mData);
		return *this;
	}

	forceinline Shared& operator=(null_t)
	{
		Shared temp(mData);
		mData = null;
		return *this;
	}

	constexpr forceinline T* Ptr() const noexcept {return mData? &mData->Value: null;}
	constexpr forceinline T* get() const noexcept {return Ptr();}

	template<typename... Args> forceinline static Shared New(Args&&... args)
	{return new Data(Cpp::Forward<Args>(args)...);}

	forceinline uint use_count() const
	{
		if(mData == null) return 0;
		return mData->GetRC();
	}

	forceinline T& operator*() const {INTRA_DEBUG_ASSERT(mData != null); return mData->Value;}
	forceinline T* operator->() const {INTRA_DEBUG_ASSERT(mData != null); return &mData->Value;}

	constexpr forceinline bool operator==(null_t) const {return mData == null;}
	constexpr forceinline bool operator!=(null_t) const {return !operator==(null);}
	constexpr forceinline bool operator==(const Shared& rhs) const {return mData == rhs.mData;}
	constexpr forceinline bool operator!=(const Shared& rhs) const {return !operator==(rhs);}

	constexpr forceinline explicit operator bool() const {return mData != null;}

private:
	Data* mData;
};

template<typename T> class SharedClass
{
	void* operator new(size_t bytes) = delete;
	void operator delete(void* ptr, size_t bytes) = delete;

	typedef typename Shared<T>::Data DerivedData;

public:
	//! Получить умный указатель Shared из этого экземпляра класса.
	//! Если уже запущено удаление экземпляра класса вследствие обнуления счётчика ссылок, вернёт null.
	//! Этот факт может использоваться в случае, когда деструктор объекта ждёт, пока текущий поток освободит ресурс.
	//! Нельзя использовать с уже удалённым объектом.
	forceinline Shared<T> SharedThis()
	{
		void* const address = reinterpret_cast<char*>(this) - Meta::MemberOffset(&DerivedData::Value);
		const auto data = static_cast<DerivedData*>(address);
		if(data->IncRef()) return data;

		//Счётчик ссылок уже нулевой, объект находится в процессе удаления.
		return null;
	}
};

template<typename T> forceinline Shared<Meta::RemoveReference<T>> SharedMove(T&& rhs)
{return Shared<Meta::RemoveReference<T>>::New(Cpp::Move(rhs));}

INTRA_WARNING_POP

}
using Utils::Shared;
using Utils::SharedMove;
using Utils::SharedClass;

}
