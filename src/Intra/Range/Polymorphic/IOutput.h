#pragma once

#include "Intra/Core.h"

INTRA_BEGIN
template<typename T> struct Span;

template<typename T> class IOutput
{
public:
	virtual ~IOutput() {}

	virtual bool Full() const = 0;
	virtual void Put(const T& value) = 0;
	virtual void Put(T&& value) = 0;

	//! It is not necessary to override this method but it reduces the virtual call number in 2 times.
	virtual bool TryPut(const T& value)
	{
		if(Full()) return false;
		Put(value);
		return true;
	}

	//! It is not necessary to override this method but it reduces the virtual call number in 2 times.
	virtual bool TryPut(T&& value)
	{
		if(Full()) return false;
		Put(Move(value));
		return true;
	}
};

template<typename T> class IOutputEx: public IOutput<T>
{
public:
	virtual size_t PutAllAdvance(Span<const T>& src) = 0;
};

using IOutputStream = IOutputEx<char>;
INTRA_END
