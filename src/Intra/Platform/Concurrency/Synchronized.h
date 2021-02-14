#pragma once

#include "Mutex.h"
#include "Lock.h"

namespace Intra { INTRA_BEGIN
#if(INTRA_LIBRARY_MUTEX != INTRA_LIBRARY_MUTEX_None)
template<typename T> class Synchronized: private Mutex
{
	template<typename U> friend class Lock;

	struct LockRef: ::Intra::Lock<Mutex>
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
	template<typename... Args> Synchronized(Args&&... args): Value(Forward<Args>(args)...) {}
	INTRA_FORCEINLINE LockRef operator->() {return {*this};}

	Synchronized(Synchronized&& rhs): Value(Move(rhs.Value)) {}
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
		Value = Move(rhs.Value);
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
		return Value == Forward<U>(rhs);
	}

	template<typename U> bool operator!=(U&& rhs)
	{
		auto l = MakeLock(this);
		return Value != Forward<U>(rhs);
	}

#define INTRA_SYNCHRONIZED_OP(op) template<typename U> Synchronized& operator op(U&& rhs)\
	{\
		INTRA_SYNCHRONIZED(this) Value op Forward<U>(rhs);\
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
#else
template<typename T> class Synchronized;
#endif

} INTRA_END
