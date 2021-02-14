#pragma once

#include "Intra/Assert.h"
#include "Intra/Concurrency/Atomic.h"

namespace Intra { INTRA_BEGIN
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
		template<typename... Args> Data(Args&&... args): Value(INTRA_FWD(args)...), RefCount(1) {}

		void Release()
		{
			INTRA_PRECONDITION(GetRC() != 0);
			if(DecRef()) delete this;
		}
		
		T Value;

		Atomic<int> RefCount;
		unsigned IncRef() {return unsigned(RefCount.GetAdd<MemoryOrder::Relaxed>(1));}
		unsigned GetRC() {return unsigned(RefCount.Get<MemoryOrder::Relaxed>());}
		bool DecRef() {return RefCount.Sub<MemoryOrder::AcquireRelease>(1) == 0;}

		Data(const Data&) = delete;
		Data& operator=(const Data&) = delete;
	};
	constexpr Shared(Data* data): mData(data) {}
public:
	Shared() = default;
	constexpr Shared(decltype(nullptr)) {}

	Shared(const Shared& rhs): mData(rhs.mData) {if(mData != nullptr) mData->IncRef();}

	template<CDerived<T> U> //requires CHasVirtualDestructor<T>
	Shared(const Shared<U>& rhs): mData(rhs.mData) {if(mData != nullptr) mData->IncRef();}

	Shared(Shared&& rhs): mData(rhs.mData) {rhs.mData = nullptr;}
	
	template<CDerived<T> U> //requires CHasVirtualDestructor<T>
	Shared(Shared<U>&& rhs): mData(rhs.mData) {rhs.mData = nullptr;}

	~Shared() {if(mData != nullptr) mData->Release();}

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

	Shared& operator=(decltype(nullptr))
	{
		Swap(*this, Shared());
		return *this;
	}

	constexpr T* Ptr() const noexcept {return mData? &mData->Value: nullptr;}
	constexpr T* get() const noexcept {return Ptr();}

	template<typename... Args> static Shared New(Args&&... args)
	{return new Data(INTRA_FWD(args)...);}

	unsigned use_count() const
	{
		if(mData == nullptr) return 0;
		return mData->GetRC();
	}

	T& operator*() const
	{
		INTRA_PRECONDITION(*this != nullptr);
		return mData->Value;
	}

	T* operator->() const
	{
		INTRA_PRECONDITION(*this != nullptr);
		return &mData->Value;
	}

	constexpr bool operator==(decltype(nullptr)) const {return mData == nullptr;}
	constexpr bool operator!=(decltype(nullptr)) const {return !operator==(nullptr);}
	bool operator==(const Shared& rhs) const = default;

	constexpr explicit operator bool() const {return mData != nullptr;}

private:
	Data* mData = nullptr;
};

template<typename T> class SharedClass
{
	void* operator new(size_t bytes) = delete;
	void operator delete(void* ptr, size_t bytes) = delete;

	using DerivedData = typename Shared<T>::Data;

public:
	/// Получить умный указатель Shared из этого экземпляра класса.
	/// Если уже запущено удаление экземпляра класса вследствие обнуления счётчика ссылок, вернёт nullptr.
	/// Этот факт может использоваться в случае, когда деструктор объекта ждёт, пока текущий поток освободит ресурс.
	/// Нельзя использовать с уже удалённым объектом.
	Shared<T> SharedThis()
	{
		void* const address = reinterpret_cast<char*>(this) - __builtin_offsetof(DerivedData, Value);
		const auto data = static_cast<DerivedData*>(address);
		if(data->IncRef()) return data;

		//Счётчик ссылок уже нулевой, объект находится в процессе удаления.
		return nullptr;
	}
};

template<typename T> Shared<TRemoveReference<T>> SharedMove(T&& rhs)
{return Shared<TRemoveReference<T>>::New(Move(rhs));}
} INTRA_END
