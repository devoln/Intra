#pragma once

//! Используется для отключения кода препроцессором: #if INTRA_DISABLED ... #endif
#define INTRA_DISABLED 0

#include "CompilerSpecific/CompilerSpecific.h"
#include "Platform/PlatformInfo.h"
#include "Core/FundamentalTypes.h"
#include "Meta/Type.h"

#ifndef _MSC_VER
#include <string.h>
#include <stdlib.h>
#endif

namespace Intra { namespace core {
	template<typename T> constexpr forceinline Meta::RemoveReference<T>&& move(T&& t)
	{
		return static_cast<Meta::RemoveReference<T>&&>(t);
	}

	template<typename T> constexpr forceinline T&& forward(Meta::RemoveReference<T>& t)
	{
		return static_cast<T&&>(t);
	}

	template<typename T> constexpr forceinline T&& forward(Meta::RemoveReference<T>&& t)
	{
		static_assert(!Meta::IsLValueReference<T>::_, "bad forward call");
		return static_cast<T&&>(t);
	}

	template<typename T> forceinline void swap(T& a, T& b)
	{
		T temp = move(a);
		a = move(b);
		b = move(temp);
	}

#ifdef _MSC_VER
#pragma warning(push, 0)
#endif
	template<typename T1, typename T2> struct pair
	{
		T1 first;
		T2 second;
	};
#ifdef _MSC_VER
#pragma warning(pop)
#endif

	template<typename T, size_t N> constexpr forceinline size_t numof(const T(&)[N]) {return N;}

	template<class T, typename U> constexpr forceinline size_t member_offset(U T::* member)
	{
		return reinterpret_cast<size_t>(&((static_cast<T*>(nullptr))->*member));
	}

	template<class T> forceinline T* addressof(T& arg)
	{
		return reinterpret_cast<T*>(&const_cast<char&>(reinterpret_cast<const volatile char &>(arg)));
	}

	extern "C"
	{
#ifdef _MSC_VER
		const void* INTRA_CRTDECL memchr(const void* buf, int val, size_t maxCount);
		int INTRA_CRTDECL memcmp(const void* buf1, const void* buf2, size_t size);
		void* INTRA_CRTDECL memcpy(void* dst, const void* src, size_t size);
		void* INTRA_CRTDECL memmove(void* _Dst, const void* src, size_t size);
		void* INTRA_CRTDECL memset(void*, int val, size_t size);
		size_t INTRA_CRTDECL strlen(const char* str) throw();
		size_t INTRA_CRTDECL wcslen(const wchar_t* str) throw();

		void* INTRA_CRTDECL malloc(size_t bytes) throw();
		void* INTRA_CRTDECL realloc(void* oldPtr, size_t bytes) throw();
		void INTRA_CRTDECL free(void* ptr) throw();
#else
		using ::memchr;
		using ::memcmp;
		using ::memcpy;
		using ::memmove;
		using ::memset;
		using ::strlen;
		using ::wcslen;
		using ::malloc;
		using ::realloc;
		using ::free;
#endif
	}
}}

#ifdef _MSC_VER

#ifndef __PLACEMENT_NEW_INLINE
#define __PLACEMENT_NEW_INLINE
inline void* operator new(size_t, void* dst) {return dst;}
inline void operator delete(void*, void*) {return;}
#endif

#ifndef __PLACEMENT_VEC_NEW_INLINE
#define __PLACEMENT_VEC_NEW_INLINE
inline void* operator new[](size_t, void* dst) {return dst;}
inline void operator delete[](void*, void*) {}
#endif

#else
#include <new>
#endif

#define INTRA_CHECK_TABLE_SIZE(table, expectedSize) static_assert(\
	sizeof(table)/sizeof(table[0])==size_t(expectedSize), "Table is outdated!")


#include "Debug.h"
#include "Math/MathEx.h"




