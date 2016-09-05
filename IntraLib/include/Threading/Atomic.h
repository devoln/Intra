#pragma once

#if(INTRA_LIBRARY_THREADING==INTRA_LIBRARY_THREADING_CPPLIB)
#include <atomic>
namespace Intra {

template<typename T> struct Atomic: public std::atomic<T>
{
	Atomic() noexcept {}
	Atomic(T val) noexcept: value(val) {}
	Atomic(const T&) = delete;

	forceinline T Load() noexcept {return load();}
	forceinline T Exchange(T val) noexcept {return exchange(val);}
	forceinline void Store(T val) noexcept {store(val);}
	forceinline bool CompareExchangeWeak(T& expected, T desired) {compare_exchange_weak(expected, desired);}

	using std::atomic<T>::operator=(T rhs);
};

}
#elif(INTRA_LIBRARY_THREADING==INTRA_LIBRARY_THREADING_Dummy)


namespace Intra {

template<typename T> struct Atomic
{
	Atomic() noexcept {}
	Atomic(T val) noexcept: value(val) {}
	Atomic(const T&) = delete;

	T Load() noexcept {return value;}
	T load() noexcept {return Load();}
	T Exchange(T val) noexcept {T result = value; value=val; return result;}
	T exchange(T val) noexcept {return Exchange(val);}
	void Store(T val) noexcept {value = val;}
	//bool CompareExchangeWeak(T& expected, T desired) {}
	//bool compare_exchange_weak(T& expected, T desired) {return CompareExchangeWeak(expected, desired);}

	T operator++() {return ++value;}
	T operator++() volatile {return ++value;}
	T operator++(int) {return value++;}
	T operator++(int) volatile {return value++;}
	T operator--() {return --value;}
	T operator--() volatile {return --value;}
	T operator--(int) {return value--;}
	T operator--(int) volatile {return value--;}

	T operator=(T rhs) {return value = rhs;}
	Atomic& operator=(const Atomic&) = delete;

	T operator+=(T arg) {return value+=arg;}
	T operator+=(T arg) volatile {return value+=arg;}
	T operator-=(T arg) {return value-=arg;}
	T operator-=(T arg) volatile {return value-=arg;}
	T operator&=(T arg) {return value&=arg;}
	T operator&=(T arg) volatile {return value&=arg;}
	T operator|=(T arg) {return value|=arg;}
	T operator|=(T arg) volatile {return value|=arg;}
	T operator^=(T arg) {return value^=arg;}
	T operator^=(T arg) volatile {return value^=arg;}

	operator T() const noexcept {return value;}

private:
	T value;
};

}
#else
#include "Threading/Thread.h"

namespace Intra {

//TODO: добавить специализации шаблона Atomic для некоторых типов, используя compiler intrinsics.

template<typename T> struct Atomic
{
	Atomic() noexcept {}
	Atomic(T val) noexcept: value(val) {}
	Atomic(const Atomic&) = delete;

	T Load() const noexcept {Intra::Mutex::Locker l(mutex); return value;}
	T load() const noexcept {return Load();}
	T Exchange(T val) noexcept {Intra::Mutex::Locker l(mutex); {T result = value; value=val; return result;}}
	T exchange(T val) noexcept {return Exchange(val);}
	void Store(T val) noexcept {Intra::Mutex::Locker l(mutex); value = val;}
	void store(T val) noexcept {Store(val);}
	//bool CompareExchangeWeak(T& expected, T desired) {}
	//bool compare_exchange_weak(T& expected, T desired) {return CompareExchangeWeak(expected, desired);}

	T operator++() {Intra::Mutex::Locker l(mutex); return ++value;}
	T operator++() volatile {Intra::Mutex::Locker l(mutex); return ++value;}
	T operator++(int) {Intra::Mutex::Locker l(mutex); return value++;}
	T operator++(int) volatile {Intra::Mutex::Locker l(mutex); return value++;}
	T operator--() {Intra::Mutex::Locker l(mutex); return --value;}
	T operator--() volatile {Intra::Mutex::Locker l(mutex); return --value;}
	T operator--(int) {Intra::Mutex::Locker l(mutex); return value--;}
	T operator--(int) volatile {Intra::Mutex::Locker l(mutex); return value--;}

	T operator=(T rhs) {Intra::Mutex::Locker l(mutex); return value = rhs;}
	Atomic& operator=(const Atomic&) = delete;

	T operator+=(T arg) {Intra::Mutex::Locker l(mutex); return value+=arg;}
	T operator+=(T arg) volatile {Intra::Mutex::Locker l(mutex); return value+=arg;}
	T operator-=(T arg) {Intra::Mutex::Locker l(mutex); return value-=arg;}
	T operator-=(T arg) volatile {Intra::Mutex::Locker l(mutex); return value-=arg;}
	T operator&=(T arg) {Intra::Mutex::Locker l(mutex); return value&=arg;}
	T operator&=(T arg) volatile {Intra::Mutex::Locker l(mutex); return value&=arg;}
	T operator|=(T arg) {Intra::Mutex::Locker l(mutex); return value|=arg;}
	T operator|=(T arg) volatile {Intra::Mutex::Locker l(mutex); return value|=arg;}
	T operator^=(T arg) {Intra::Mutex::Locker l(mutex); return value^=arg;}
	T operator^=(T arg) volatile {Intra::Mutex::Locker l(mutex); return value^=arg;}

	operator T() const noexcept {return Load();}

private:
	T value;
	mutable Mutex mutex;
};

}

#endif
