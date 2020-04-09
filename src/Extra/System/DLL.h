#pragma once

#include "Intra/Range/StringView.h"

INTRA_BEGIN
class DLL
{
	struct Data;
	Data* mHandle;
public:
	typedef Data* NativeHandle;

	DLL(decltype(null) = null): mHandle(null) {}
	DLL(DLL&& rhs): mHandle(rhs.mHandle) {rhs.mHandle = null;}

	DLL& operator=(DLL&& rhs)
	{
		if(this == &rhs) return *this;
		Unload();
		mHandle = rhs.mHandle;
		rhs.mHandle = null;
		return *this;
	}

	INTRA_FORCEINLINE DLL& operator=(decltype(null)) {Unload(); return *this;}
	
	~DLL() {Unload();}

	DLL(const DLL&) = delete;
	DLL& operator=(const DLL&) = delete;

	static DLL Load(StringView dllName);
	void* FunctionAddress(StringView name) const;
	void Unload();

	template<typename F> INTRA_FORCEINLINE F* Function(StringView name)
	{return static_cast<F*>(FunctionAddress(name));}

	template<typename F> INTRA_FORCEINLINE void Function(StringView name, F*& f)
	{return f = static_cast<F*>(FunctionAddress(name));}

	INTRA_FORCEINLINE NativeHandle GetNativeHandle() const {return mHandle;}
};
INTRA_END
