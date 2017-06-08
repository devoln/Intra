﻿#pragma once

#include "Cpp/PlatformDetect.h"
#include "Cpp/Features.h"
#include "Cpp/Warnings.h"

#include "Thread.h"

#define INTRA_LIBRARY_ATOMIC_Dummy 0
#define INTRA_LIBRARY_ATOMIC_WinAPI 1
#define INTRA_LIBRARY_ATOMIC_CPPLIB 2
#define INTRA_LIBRARY_ATOMIC_Mutex 3

#ifndef INTRA_LIBRARY_ATOMIC

/*#if(INTRA_PLATFORM_OS == INTRA_PLATFORM_OS_Windows)

#define INTRA_LIBRARY_ATOMIC INTRA_LIBRARY_ATOMIC_WinAPI
*/
#if(INTRA_PLATFORM_OS == INTRA_PLATFORM_OS_Emscripten)

/*#ifdef __EMSCRIPTEN_PTHREADS__
#define INTRA_LIBRARY_ATOMIC INTRA_LIBRARY_ATOMIC_PThread
#else*/
#define INTRA_LIBRARY_ATOMIC INTRA_LIBRARY_ATOMIC_Dummy
//#endif

#else
#define INTRA_LIBRARY_ATOMIC INTRA_LIBRARY_ATOMIC_Mutex
#endif

#endif

INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS

#if(INTRA_LIBRARY_ATOMIC == INTRA_LIBRARY_ATOMIC_CPPLIB)
#include <atomic>
namespace Intra { namespace Concurrency {

template<typename T> struct Atomic: public std::atomic<T>
{
	Atomic() noexcept {}
	Atomic(T val) noexcept: std::atomic<T>(val) {}
	Atomic(const Atomic&) = delete;

	forceinline T Load() const noexcept {return load();}
	forceinline T Exchange(T val) noexcept {return exchange(val);}
	forceinline void Store(T val) noexcept {store(val);}
	forceinline bool CompareExchangeWeak(T& expected, T desired) {compare_exchange_weak(expected, desired);}

	using std::atomic<T>::operator=(T rhs);
};

}}
#elif(INTRA_LIBRARY_ATOMIC == INTRA_LIBRARY_ATOMIC_Dummy)


namespace Intra { namespace Concurrency {

template<typename T> struct Atomic
{
	forceinline Atomic() noexcept {}
	forceinline Atomic(T val) noexcept: mValue(val) {}
	Atomic(const Atomic&) = delete;

	forceinline T Load() const noexcept {return mValue;}
	forceinline T load() const noexcept {return Load();}
	forceinline T Exchange(T val) noexcept {T result = mValue; mValue=val; return result;}
	forceinline T exchange(T val) noexcept {return Exchange(val);}
	forceinline void Store(T val) noexcept {mValue = val;}
	//forceinline bool CompareExchangeWeak(T& expected, T desired) {}
	//forceinline bool compare_exchange_weak(T& expected, T desired) {return CompareExchangeWeak(expected, desired);}

	forceinline T operator++() {return ++mValue;}
	forceinline T operator++() volatile {return ++mValue;}
	forceinline T operator++(int) {return mValue++;}
	forceinline T operator++(int) volatile {return mValue++;}
	forceinline T operator--() {return --mValue;}
	forceinline T operator--() volatile {return --mValue;}
	forceinline T operator--(int) {return mValue--;}
	forceinline T operator--(int) volatile {return mValue--;}

	forceinline T operator=(T rhs) {return mValue = rhs;}
	forceinline Atomic& operator=(const Atomic&) = delete;

	forceinline T operator+=(T arg) {return mValue += arg;}
	forceinline T operator+=(T arg) volatile {return mValue += arg;}
	forceinline T operator-=(T arg) {return mValue -= arg;}
	forceinline T operator-=(T arg) volatile {return mValue -= arg;}
	forceinline T operator&=(T arg) {return mValue &= arg;}
	forceinline T operator&=(T arg) volatile {return mValue &= arg;}
	forceinline T operator|=(T arg) {return mValue |= arg;}
	forceinline T operator|=(T arg) volatile {return mValue |= arg;}
	forceinline T operator^=(T arg) {return mValue ^= arg;}
	forceinline T operator^=(T arg) volatile {return mValue ^= arg;}

	forceinline operator T() const noexcept {return mValue;}

private:
	T mValue;
};

template<typename T> struct Atomic<T*>
{
	forceinline Atomic() noexcept {}
	forceinline Atomic(T* val) noexcept: mValue(val) {}
	Atomic(const Atomic&) = delete;

	forceinline T* Load() const noexcept {return mValue;}
	forceinline T* load() const noexcept {return Load();}
	forceinline T* Exchange(T* val) noexcept {T* result = mValue; mValue = val; return result;}
	forceinline T* exchange(T* val) noexcept {return Exchange(val);}
	forceinline void Store(T* val) noexcept {mValue = val;}
	//forceinline bool CompareExchangeWeak(T*& expected, T* desired) {}
	//forceinline bool compare_exchange_weak(T*& expected, T* desired) {return CompareExchangeWeak(expected, desired);}

	forceinline T operator++() {return ++mValue;}
	forceinline T operator++() volatile {return ++mValue;}
	forceinline T operator++(int) {return mValue++;}
	forceinline T operator++(int) volatile {return mValue++;}
	forceinline T operator--() {return --mValue;}
	forceinline T operator--() volatile {return --mValue;}
	forceinline T operator--(int) {return mValue--;}
	forceinline T operator--(int) volatile {return mValue--;}

	forceinline T operator=(T* rhs) {return mValue = rhs;}
	forceinline Atomic& operator=(const Atomic&) = delete;

	forceinline T* operator+=(T* arg) {return mValue += arg;}
	forceinline T* operator+=(T* arg) volatile {return mValue += arg;}
	forceinline T* operator-=(T* arg) {return mValue -= arg;}
	forceinline T* operator-=(T* arg) volatile {return mValue -= arg;}

	forceinline operator T*() const noexcept {return mValue;}
	forceinline T* operator->() const noexcept {return mValue;}

private:
	T* mValue;
};

}}
#elif(INTRA_LIBRARY_ATOMIC == INTRA_LIBRARY_ATOMIC_Mutex)
#include "Thread.h"
#include "Mutex.h"
#include "Lock.h"

namespace Intra { namespace Concurrency {

//TODO: добавить специализации шаблона Atomic для некоторых типов, используя compiler intrinsics.

template<typename T> struct Atomic
{
	Atomic() noexcept {}
	Atomic(T val) noexcept: mValue(val) {}
	Atomic(const Atomic&) = delete;

	T Load() const noexcept {auto locker = MakeLock(mMutex); return mValue;}
	T load() const noexcept {return Load();}
	T Exchange(T val) noexcept {auto locker = MakeLock(mMutex); {T result = mValue; mValue = val; return result;}}
	T exchange(T val) noexcept {return Exchange(val);}
	void Store(T val) noexcept {auto locker = MakeLock(mMutex); mValue = val;}
	void store(T val) noexcept {Store(val);}
	//bool CompareExchangeWeak(T& expected, T desired) {}
	//bool compare_exchange_weak(T& expected, T desired) {return CompareExchangeWeak(expected, desired);}

	T operator++() {auto locker = MakeLock(mMutex); return ++mValue;}
	T operator++(int) {auto locker = MakeLock(mMutex); return mValue++;}
	T operator--() {auto locker = MakeLock(mMutex); return --mValue;}
	T operator--(int) {auto locker = MakeLock(mMutex); return mValue--;}

	T operator=(T rhs) {auto locker = MakeLock(mMutex); return mValue = rhs;}
	Atomic& operator=(const Atomic&) = delete;

	T operator+=(T arg) {auto locker = MakeLock(mMutex); return mValue += arg;}
	T operator-=(T arg) {auto locker = MakeLock(mMutex); return mValue -= arg;}
	T operator&=(T arg) {auto locker = MakeLock(mMutex); return mValue &= arg;}
	T operator|=(T arg) {auto locker = MakeLock(mMutex); return mValue |= arg;}
	T operator^=(T arg) {auto locker = MakeLock(mMutex); return mValue ^= arg;}

	operator T() const noexcept {return Load();}

private:
	T mValue;
	mutable Mutex mMutex;
};

template<typename T> struct Atomic<T*>
{
	Atomic() noexcept {}
	Atomic(T* val) noexcept: mValue(val) {}
	Atomic(const Atomic&) = delete;

	T* Load() const noexcept {auto locker = MakeLock(mMutex); return mValue;}
	T* load() const noexcept {return Load();}

	T* Exchange(T* val) noexcept
	{
		auto locker = MakeLock(mMutex);
		T* result = mValue;
		mValue = val;
		return result;
	}

	T* exchange(T* val) noexcept {return Exchange(val);}
	void Store(T* val) noexcept {auto locker = MakeLock(mMutex); mValue = val;}
	void store(T* val) noexcept {Store(val);}
	//bool CompareExchangeWeak(T*& expected, T* desired) {}
	//bool compare_exchange_weak(T*& expected, T* desired) {return CompareExchangeWeak(expected, desired);}

	T* operator++() {auto locker = MakeLock(mMutex); return ++mValue;}
	T* operator++(int) {auto locker = MakeLock(mMutex); return mValue++;}
	T* operator--() {auto locker = MakeLock(mMutex); return --mValue;}
	T* operator--(int) {auto locker = MakeLock(mMutex); return mValue--;}

	T* operator=(T* rhs) {auto locker = MakeLock(mMutex); return mValue = rhs;}
	Atomic& operator=(const Atomic&) = delete;

	T* operator+=(T arg) {auto locker = MakeLock(mMutex); return mValue += arg;}
	T* operator-=(T arg) {auto locker = MakeLock(mMutex); return mValue -= arg;}

	operator T*() const noexcept {return Load();}
	T* operator->() const noexcept {return Load();}

private:
	T* mValue;
	mutable Mutex mMutex;
};

}}

#endif

namespace Intra {
using Concurrency::Atomic;
}

INTRA_WARNING_POP
