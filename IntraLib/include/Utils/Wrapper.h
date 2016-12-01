#pragma once

#include "Memory/SmartRef.h"

namespace Intra { namespace Utils {

template<typename T> struct Wrapper: Memory::UniqueRef<T>
{
private:
	typedef Memory::UniqueRef<T> base;
protected:
	Wrapper(T* ptr): base(ptr) {}
public:
	Wrapper(const Wrapper& rhs) = delete;
	Wrapper(Wrapper&& rhs): base(core::move(rhs)) {}

	static Wrapper CapturePtr(T* ptr) {return Wrapper(ptr);}

	Wrapper& operator=(const Wrapper& rhs) = delete;

	Wrapper& operator=(Wrapper&& rhs)
	{
		base::operator=(core::move(rhs));
		return *this;
	}
};

template<typename T> struct CloneableWrapper: Memory::UniqueRef<T>
{
private:
	typedef Memory::UniqueRef<T> base;
protected:
	CloneableWrapper(T* ptr): base(ptr) {}

public:
	CloneableWrapper(const CloneableWrapper& rhs): base(rhs->Clone()) {}
	CloneableWrapper(CloneableWrapper&& rhs): base(core::move(rhs)) {}

	static CloneableWrapper CapturePtr(T* ptr) {return CloneableWrapper(ptr);}

	CloneableWrapper& operator=(const CloneableWrapper& rhs)
	{
		base::operator=(rhs->Clone());
		return *this;
	}

	CloneableWrapper& operator=(CloneableWrapper&& rhs)
	{
		base::operator=(core::move(rhs));
		return *this;
	}

	template<typename U=T> decltype(Meta::Val<const U>()[size_t()]) operator[](size_t index) const {return (**this)[index];}
	template<typename U=T> decltype(Meta::Val<U>()[size_t()]) operator[](size_t index) {return (**this)[index];}
};

/*template<typename T> struct CloneableIndexableHolder: CloneableWrapper<T>
{
	CloneableIndexableHolder(const CloneableIndexableHolder& rhs): CloneableWrapper<T>(rhs) {}
	CloneableIndexableHolder(CloneableIndexableHolder&& rhs): CloneableWrapper<T>(core::move(rhs)) {}
	template<typename R> CloneableIndexableHolder(R range): CloneableWrapper<T>(range) {}

	static CloneableIndexableHolder CapturePtr(T* ptr) {return CloneableIndexableHolder(ptr);}

	template<typename U=T> decltype(Meta::Val<const U>()[size_t()]) operator[](size_t index) const {return (**this)[index];}
	template<typename U=T> decltype(Meta::Val<U>()[size_t()]) operator[](size_t index) {return (**this)[index];}
};*/

}}
