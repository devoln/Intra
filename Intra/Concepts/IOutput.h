#pragma once

#include "Cpp/Fundamental.h"

namespace Intra {

namespace Range {
template<typename T> struct Span;

template<typename T> class IOutput
{
public:
	virtual ~IOutput() {}

	virtual bool Full() const = 0;
	virtual void Put(const T& value) = 0;
	virtual void Put(T&& value) = 0;

	//! Переопределить для повышения производительности за счёт меньшего числа виртуальных вызовов.
	virtual bool TryPut(const T& value)
	{
		if(Full()) return false;
		Put(value);
		return true;
	}

	//! Переопределить для повышения производительности за счёт меньшего числа виртуальных вызовов.
	virtual bool TryPut(T&& value)
	{
		if(Full()) return false;
		Put(Cpp::Move(value));
		return true;
	}
};

template<typename T> class IOutputEx: public IOutput<T>
{
public:
	virtual size_t PutAllAdvance(Span<const T>& src) = 0;
};

typedef IOutputEx<char> IOutputStream;

}

namespace Concepts {
using Range::IOutput;
using Range::IOutputEx;
using Range::IOutputStream;
}

using Range::IOutput;
using Range::IOutputEx;
using Range::IOutputStream;

}
