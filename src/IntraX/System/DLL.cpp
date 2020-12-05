#include "DLL.h"

#include "IntraX/Utils/FixedArray.h"

#include "detail/Common.h"

INTRA_PUSH_DISABLE_ALL_WARNINGS
#ifdef _WIN32

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <Windows.h>

#else
#include <dlfcn.h>
#endif
INTRA_WARNING_POP

INTRA_BEGIN
#ifdef _WIN32
void DLL::Unload()
{
	if(mHandle == null) return;
	FreeLibrary(HMODULE(mHandle));
}

#if defined(WINAPI_FAMILY_PARTITION) && defined(WINAPI_PARTITION_APP)
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_APP) && _WIN32_WINNT >= 0x0602
#define WINSTORE_LPL
#endif
#endif

DLL DLL::Load(StringView dllName)
{
	FixedArray<wchar_t> wdllName(dllName.Length() + 1);
	const int wlen = MultiByteToWideChar(CP_UTF8, 0, dllName.Data(),
		int(dllName.Length()), wdllName.Data(), int(wdllName.Length()));
	wdllName[wlen] = L'\0';

	DLL result;

#ifdef WINSTORE_LPL
	result.mHandle = NativeHandle(LoadPackagedLibrary(wdllName.Data(), 0));
#undef WINSTORE_LPL
#else
	result.mHandle = NativeHandle(LoadLibraryW(wdllName.Data()));
#endif
	return result;
}

void* DLL::FunctionAddress(StringView name) const
{
	FixedArray<char> zname(name.Length() + 1);
	name.CopyTo(zname);
	zname.Last() = '\0';
	return reinterpret_cast<void*>(GetProcAddress(HMODULE(mHandle), zname.Data()));
}
#else
void DLL::Unload()
{
	if(mHandle == null) return;
	dlclose(mHandle);
}

DLL DLL::Load(StringView dllName)
{
	FixedArray<char> zdllName(dllName.Length() + 1);
	dllName.CopyTo(zdllName);
	zdllName.Last() = '\0';
	DLL result;
	result.mHandle = NativeHandle(dlopen(zdllName.Data(), RTLD_LOCAL));
	return result;
}

void* DLL::FunctionAddress(StringView name) const
{
	FixedArray<char> zname(name.Length() + 1);
	name.CopyTo(zname);
	zname.Last() = '\0';
	return dlsym(mHandle, zname.Data());
}
#endif
INTRA_END
