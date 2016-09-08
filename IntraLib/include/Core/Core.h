#pragma once

//! Используется для отключения кода препроцессором: #if INTRA_DISABLED ... #endif
#define INTRA_DISABLED 0

#include "CompilerSpecific/CompilerSpecific.h"
#include "Platform/PlatformInfo.h"
#include "Core/FundamentalTypes.h"
#include "Meta/Type.h"

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

#pragma warning(push, 0)
	template<typename T1, typename T2> struct pair
	{
		T1 first;
		T2 second;
	};
#pragma warning(pop)

	template<typename T, size_t N> constexpr forceinline size_t numof(const T(&)[N]) {return N;}
	template<class T, typename U> constexpr forceinline size_t member_offset(U T::* member)
	{
		return reinterpret_cast<size_t>(&(((T*)nullptr)->*member));
	}

	template<class T> forceinline T* addressof(T& arg)
	{
		return reinterpret_cast<T*>(&const_cast<char&>(reinterpret_cast<const volatile char &>(arg)));
	}

	extern "C"
	{
		const void* INTRA_CRTDECL memchr(const void* buf, int val, size_t maxCount);
		int INTRA_CRTDECL memcmp(const void* buf1, const void* buf2, size_t size);
		void* INTRA_CRTDECL memcpy(void* dst, const void* src, size_t size);
		void* INTRA_CRTDECL memmove(void* _Dst, const void* src, size_t size);
		void* INTRA_CRTDECL memset(void*, int val, size_t size);
		size_t INTRA_CRTDECL strlen(const char* str);
		size_t INTRA_CRTDECL wcslen(const wchar_t* str);

		void* INTRA_CRTDECL malloc(size_t bytes);
		void* INTRA_CRTDECL realloc(void* oldPtr, size_t bytes);
		void INTRA_CRTDECL free(void* ptr);
	}
}}

#define INTRA_CHECK_TABLE_SIZE(table, expectedSize) static_assert(sizeof(table)/sizeof(table[0])==size_t(expectedSize), "Table is outdated!")


#include "Debug.h"
#include "Math/MathEx.h"




