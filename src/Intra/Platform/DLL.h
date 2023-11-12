#pragma once

#include <Intra/Platform/Toolchain.h>
#include <Intra/Range/StringView.h>
#include <Intra/Container/Array.h>

namespace Intra { INTRA_BEGIN
class DynamicLibrary
{
	HandleMovableWrapper<void*> mHandle = nullptr;
public:
	DynamicLibrary() = default;
	DynamicLibrary(ZStringView dllName)
	{
		LocalArray<wchar_t> wdllName(dllName.Length() + 1);
		const int wlen = z_D::MultiByteToWideChar(65001, 0, dllName.Data(),
			int(dllName.Length()), wdllName.Data(), int(wdllName.Length()));
		wdllName[wlen] = L'\0';

		DynamicLibrary res;
#ifdef _WIN32
#if INTRA_WINSTORE_APP
		res.mHandle = z_D::LoadPackagedLibrary(wdllName.Data(), 0);
#else
		res.mHandle = z_D::LoadLibraryW(wdllName.Data());
#endif
#else
		res.mHandle = z_D::dlopen(zdllName.Data(), RTLD_LOCAL);
#endif
		return res;
	}
	DynamicLibrary(DynamicLibrary&&) = default;
	DynamicLibrary(const DynamicLibrary&) = delete;
	DynamicLibrary& operator=(DynamicLibrary&& rhs) = default;
	DynamicLibrary& operator=(const DynamicLibrary&) = delete;
	~DynamicLibrary() {Unload();}

	void* FunctionAddress(ZStringView name) const
	{
#ifdef _WIN32
		return reinterpret_cast<void*>(z_D::GetProcAddress(mHandle, name.Data()));
#else
		return dlsym(mHandle, name.Data());
#endif
	}
	void Unload();

	template<CFunction F> INTRA_FORCEINLINE F* Function(ZStringView name)
	{return static_cast<F*>(FunctionAddress(name));}

	template<CFunction F> INTRA_FORCEINLINE void Function(ZStringView name, F*& f)
	{return f = static_cast<F*>(FunctionAddress(name));}

	INTRA_FORCEINLINE auto GetNativeHandle() const {return mHandle;}
};
} INTRA_END
