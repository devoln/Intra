#pragma once

#include "Cpp/Warnings.h"
#include "Meta/Type.h"

namespace Intra {

namespace Range {
template<typename T> struct Span;

template<typename T> class IInput
{
public:
	virtual ~IInput() {}

	virtual bool Empty() const = 0;
	virtual T First() const = 0;
	virtual void PopFirst() = 0;

	//! Этот метод переопределять необязательно, но его переопределение сократит количество виртуальных вызовов.
	virtual Meta::RemoveReference<T> Next()
	{
		auto result = First();
		PopFirst();
		return result;
	}
};

template<typename T> class IInputEx: public IInput<T>
{
public:
	virtual size_t PopFirstN(size_t count)
	{
		for(size_t i=0; i<count; i++)
		{
			if(this->Empty()) return i;
			this->PopFirst();
		}
		return count;
	}

	virtual size_t ReadWrite(Span<Meta::RemoveConstRef<T>>& dst) = 0;
};

typedef IInputEx<char> IInputStream;

}
namespace Concepts {
using Range::IInput;
using Range::IInputEx;
using Range::IInputStream;
}
using Concepts::IInput;
using Concepts::IInputEx;
using Concepts::IInputStream;

}

