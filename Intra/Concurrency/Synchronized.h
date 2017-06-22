#pragma once

#include "Cpp/Fundamental.h"
#include "Mutex.h"
#include "Lock.h"

namespace Intra { namespace Concurrency {

template<typename T> class Synchronized: private Mutex
{
	template<typename U> friend class Lock;

	struct LockRef: ::Intra::Concurrency::Lock<Mutex>
	{
		T& Value;
		LockRef(Synchronized& v): Lock(v), Value(v.Value) {}
		LockRef(LockRef&&) = default;
		LockRef(const LockRef&) = delete;
		LockRef& operator=(const LockRef& v) = delete;
		T* operator->() const {return &Value;}
	};


public:
	T Value;

	Synchronized(): Value() {}
	template<typename... Args> Synchronized(Args&&... args): Value(Cpp::Forward<Args>(args)...) {}
	forceinline LockRef operator->() const {return {*const_cast<Synchronized*>(this)};}

	Synchronized(Synchronized&& rhs): Value(Cpp::Move(rhs.Value)) {}
	Synchronized(const Synchronized& rhs): Value(rhs.Value) {}

	Synchronized& operator++()
	{
		INTRA_SYNCHRONIZED(this) ++Value;
		return *this;
	}

	Synchronized& operator--()
	{
		INTRA_SYNCHRONIZED(this) --Value;
		return *this;
	}

	T operator++(int)
	{
		auto l = MakeLock(this);
		return Value++;
	}

	T operator--(int)
	{
		auto l = MakeLock(this);
		return Value--;
	}

	Synchronized& operator=(Synchronized&& rhs)
	{
		Value = Cpp::Move(rhs.Value);
		return *this;
	}

	Synchronized& operator=(const Synchronized& rhs)
	{
		Value = rhs.Value;
		return *this;
	}

	template<typename U> bool operator==(U&& rhs)
	{
		auto l = MakeLock(this);
		return Value == Cpp::Forward<U>(rhs);
	}

	template<typename U> bool operator!=(U&& rhs)
	{
		auto l = MakeLock(this);
		return Value != Cpp::Forward<U>(rhs);
	}

#define INTRA_SYNCHRONIZED_OP(op) template<typename U> Synchronized& operator op(U&& rhs)\
	{\
		INTRA_SYNCHRONIZED(this) Value op Cpp::Forward<U>(rhs);\
		return *this;\
	}

	INTRA_SYNCHRONIZED_OP(+=)
	INTRA_SYNCHRONIZED_OP(-=)
	INTRA_SYNCHRONIZED_OP(*=)
	INTRA_SYNCHRONIZED_OP(/=)
	INTRA_SYNCHRONIZED_OP(&=)
	INTRA_SYNCHRONIZED_OP(|=)
	INTRA_SYNCHRONIZED_OP(^=)
	INTRA_SYNCHRONIZED_OP(<<=)
	INTRA_SYNCHRONIZED_OP(>>=)
	INTRA_SYNCHRONIZED_OP(=)

#undef INTRA_SYNCHRONIZED_OP
};

template<typename OS, typename T> OS& operator<<(OS& stream, const Synchronized<T>& value)
{
	INTRA_SYNCHRONIZED(const_cast<Synchronized<T>&>(value)) stream << value.Value;
	return stream;
}

}
using Concurrency::Synchronized;


}
