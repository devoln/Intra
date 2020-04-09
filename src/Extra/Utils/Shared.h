#pragma once

#include "Intra/Assert.h"

#ifndef INTRA_NO_CONCURRENCY
#include "Intra/Concurrency/Atomic.h"
#endif

INTRA_BEGIN
//TODO: make a policy-based generic pointer template.

/** Lightweight shared pointer.
It doesn't take raw pointers, so the error of multiple separate reference counters for one object is not possible.
Uses new/delete for allocation and has no deleter.
No support for weak references.
Cannot be used with incomplete types.
Thread safe.
*/
template<typename T> class Shared
{
	template<typename U> friend class SharedClass;
	struct Data
	{
		template<typename... Args> Data(Args&&... args):
			Value(Forward<Args>(args)...), RefCount(1) {}

		void Release()
		{
			INTRA_PRECONDITION(GetRC() != 0);
			if(DecRef()) delete this;
		}
		
		T Value;
#if(!defined(INTRA_NO_CONCURRENCY) && INTRA_LIBRARY_ATOMIC != INTRA_LIBRARY_ATOMIC_None)
		AtomicInt RefCount;
		unsigned IncRef() {return unsigned(RefCount.GetIncrementRelaxed());}
		unsigned GetRC() {return unsigned(RefCount.GetRelaxed());}
		bool DecRef() {return RefCount.DecrementAcquireRelease() == 0;}
#else
		unsigned IncRef() {return RefCount++;}
		unsigned GetRC() {return RefCount;}
		bool DecRef() {return --RefCount == 0;}
		unsigned RefCount;
#endif

		Data(const Data&) = delete;
		Data& operator=(const Data&) = delete;
	};
	constexpr Shared(Data* data): mData(data) {}
public:
	constexpr Shared(decltype(null)=null): mData(null) {}

	Shared(const Shared& rhs): mData(rhs.mData)
	{
		if(mData != null) mData->IncRef();
	}

	template<typename U, typename = Requires<
		CDerived<U, T> // && CHasVirtualDestructor<T>
	>> Shared(const Shared<U>& rhs): mData(rhs.mData)
	{
		if(mData != null) mData->IncRef();
	}

	Shared(Shared&& rhs): mData(rhs.mData) {rhs.mData = null;}
	
	template<typename U, typename = Requires<
		CDerived<U, T> // && CHasVirtualDestructor<T>
	>> Shared(Shared<U>&& rhs): mData(rhs.mData) {rhs.mData = null;}

	~Shared() {if(mData != null) mData->Release();}

	Shared& operator=(const Shared& rhs)
	{
		if(mData == rhs.mData) return *this;
		Shared temp(rhs);
		Swap(mData, temp.mData);
		return *this;
	}

	Shared& operator=(Shared&& rhs)
	{
		Swap(mData, rhs.mData);
		return *this;
	}

	Shared& operator=(decltype(null))
	{
		Shared temp(mData);
		mData = null;
		return *this;
	}

	constexpr T* Ptr() const noexcept {return mData? &mData->Value: null;}
	constexpr T* get() const noexcept {return Ptr();}

	template<typename... Args> static Shared New(Args&&... args)
	{return new Data(Forward<Args>(args)...);}

	unsigned use_count() const
	{
		if(mData == null) return 0;
		return mData->GetRC();
	}

	T& operator*() const
	{
		INTRA_PRECONDITION(mData != null);
		return mData->Value;
	}

	T* operator->() const
	{
		INTRA_PRECONDITION(mData != null);
		return &mData->Value;
	}

	constexpr bool operator==(decltype(null)) const {return mData == null;}
	constexpr bool operator!=(decltype(null)) const {return !operator==(null);}
	constexpr bool operator==(const Shared& rhs) const {return mData == rhs.mData;}
	constexpr bool operator!=(const Shared& rhs) const {return !operator==(rhs);}

	constexpr explicit operator bool() const {return mData != null;}

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
	Shared<T> SharedThis()
	{
		void* const address = reinterpret_cast<char*>(this) - MemberOffset(&DerivedData::Value);
		const auto data = static_cast<DerivedData*>(address);
		if(data->IncRef()) return data;

		//Счётчик ссылок уже нулевой, объект находится в процессе удаления.
		return null;
	}
};

template<typename T> Shared<TRemoveReference<T>> SharedMove(T&& rhs)
{return Shared<TRemoveReference<T>>::New(Move(rhs));}
INTRA_END
