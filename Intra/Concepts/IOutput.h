#pragma once

#include "Cpp/Fundamental.h"

namespace Intra {

namespace Range {
template<typename T> struct Span;
}

namespace Utils {

template<typename T> class IOutput
{
public:
	virtual ~IOutput() {}

	virtual bool Full() const = 0;
	virtual void Put(const T& value) = 0;
	virtual void Put(T&& value) = 0;

	//! �������������� ��� ��������� ������������������ �� ���� �������� ����� ����������� �������.
	virtual bool TryPut(const T& value)
	{
		if(Full()) return false;
		Put(value);
		return true;
	}

	//! �������������� ��� ��������� ������������������ �� ���� �������� ����� ����������� �������.
	virtual bool TryPut(T&& value)
	{
		if(Full()) return false;
		Put(Cpp::Move(value));
		return true;
	}
};

template<typename T> class IOutputStream: public IOutput<T>
{
public:
	virtual size_t PutAllAdvance(Span<const T>& src) = 0;
};

}
using Utils::IOutput;
using Utils::IOutputStream;

}
