#pragma once

#include "Core/Core.h"

namespace Intra { namespace Algo {

forceinline size_t CStringLength(const char* str) {return C::strlen(str);}
forceinline size_t CStringLength(const wchar_t* str) {return C::wcslen(str);}

template<typename U=wchar> forceinline Meta::EnableIf<
	sizeof(U)==sizeof(wchar_t),
size_t> CStringLength(const wchar* str) {return C::wcslen(reinterpret_cast<const wchar_t*>(str));}

template<typename U=wchar> forceinline Meta::EnableIf<
	sizeof(U)!=sizeof(wchar_t),
size_t> CStringLength(const wchar* str)
{
	const wchar* ptr = str-1;
	while(*++ptr!=0) {}
	return size_t(ptr-str);
}

template<typename U=dchar> forceinline Meta::EnableIf<
	sizeof(U)==sizeof(wchar_t),
size_t> CStringLength(const dchar* str) {return C::wcslen(reinterpret_cast<const wchar_t*>(str));}

template<typename U=dchar> forceinline Meta::EnableIf<
	sizeof(U)!=sizeof(wchar_t),
size_t> CStringLength(const dchar* str)
{
	const dchar* ptr = str-1;
	while(*++ptr!=0) {}
	return ptr-str;
}

}}

