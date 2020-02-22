#pragma once

#include "Core/Range/StringView.h"

INTRA_BEGIN
class DLL
{
	struct Data;
	Data* mHandle;
public:
	typedef Data* NativeHandle;

	DLL(null_t = null): mHandle(null) {}
	DLL(DLL&& rhs): mHandle(rhs.mHandle) {rhs.mHandle = null;}

	DLL& operator=(DLL&& rhs)
	{
		if(this == &rhs) return *this;
		Unload();
		mHandle = rhs.mHandle;
		rhs.mHandle = null;
		return *this;
	}

	forceinline DLL& operator=(null_t) {Unload(); return *this;}
	
	~DLL() {Unload();}

	DLL(const DLL&) = delete;
	DLL& operator=(const DLL&) = delete;

	static DLL Load(StringView dllName);
	void* FunctionAddress(StringView name) const;
	void Unload();

	template<typename F> forceinline F* Function(StringView name)
	{return static_cast<F*>(FunctionAddress(name));}

	template<typename F> forceinline void Function(StringView name, F*& f)
	{return f = static_cast<F*>(FunctionAddress(name));}

	forceinline NativeHandle GetNativeHandle() const {return mHandle;}
};
INTRA_END
