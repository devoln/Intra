#pragma once

#include "Compatibility.h"
#include "Features.h"
#include "Warnings.h"
#include "PlatformDetect.h"

namespace Intra {

INTRA_PUSH_DISABLE_ALL_WARNINGS
#ifdef _MSC_VER
#pragma warning(disable: 4702)
#endif

#ifndef INTRA_NO_BASIC_TYPE_SIZE_CHECK
static_assert(sizeof(char)==1 && sizeof(short)==2 && sizeof(int)==4 && sizeof(long long)==8,
	"Some of fundamental types have unexpected size!");
#endif

enum: short {short_MIN=-32768, short_MAX=32767};
enum: int {int_MIN=int(0x80000000u), int_MAX=0x7fffffff};

typedef unsigned char byte;
enum: byte {byte_MIN=0, byte_MAX=255};

typedef signed char sbyte;
enum: sbyte {sbyte_MIN=-128, sbyte_MAX=127};

typedef unsigned short ushort;
enum: ushort {ushort_MIN=0, ushort_MAX=65535};

typedef unsigned int uint;
enum: uint {uint_MIN=0, uint_MAX=4294967295u};

typedef long long long64;
enum: long64 {long64_MIN=-9223372036854775807LL, long64_MAX=9223372036854775807LL};

typedef unsigned long long ulong64;
enum: ulong64 {ulong_MIN=0, ulong64_MAX=18446744073709551615ULL};

constexpr const float float_MAX = 3.402823466e+38F, float_MIN = 1.175494351e-38F;

//Знаковый целочисленный тип, размер зависит от разрядности платформы
typedef decltype(reinterpret_cast<char*>(1)-reinterpret_cast<char*>(0)) intptr;

//Беззнаковый целочисленный тип, размер зависит от разрядности платформы
typedef decltype(sizeof(int)) uintptr;

typedef byte flag8;                     //8-битовый флаг
typedef ushort flag16;                  //16-битовый флаг
typedef uint flag32;                    //32-битовый флаг

typedef long double real;

#ifdef INTRA_CHAR16_SUPPORT
typedef char16_t wchar;
#else
#if(!defined(__CHAR16_TYPE__) && INTRA_PLATFORM_OS == INTRA_PLATFORM_OS_Windows)
static_assert(sizeof(wchar_t)==2, "Error in platform specific wchar type definition.");
typedef wchar_t wchar;
#else
#define INTRA_WCHAR_EMULATED
struct wchar
{
	forceinline wchar() = default;
	wchar(ushort code): c(code) {}
	operator ushort() const {return c;}
	operator ushort&() {return c;}
private:
	ushort c;
};
#endif
#endif

#ifdef INTRA_CHAR32_SUPPORT
typedef char32_t dchar;
#elif(!defined(__CHAR32_TYPE__) && INTRA_PLATFORM_OS == INTRA_PLATFORM_OS_Linux)
static_assert(sizeof(wchar_t) == 4,
	"Error in platform specific dchar type definition.");
typedef wchar_t dchar;
#else
#define INTRA_DCHAR_EMULATED
struct dchar
{
	forceinline dchar() = default;
	forceinline dchar(uint code): c(code) {}
	forceinline operator uint() const {return c;}
	forceinline operator uint&() {return c;}
private:
	uint c;
};
#endif

typedef decltype(nullptr) null_t;
constexpr const null_t null = nullptr;

namespace Cpp {

template<typename T, T value> struct TypeFromValue {enum {_=value };};
typedef TypeFromValue<bool, false> FalseType;
typedef TypeFromValue<bool, true> TrueType;

namespace D {
template<typename T> struct TRemoveReference {typedef T _;};
template<typename T> struct TRemoveReference<T&> {typedef T _;};
template<typename T> struct TRemoveReference<T&&> {typedef T _;};
template<typename T> using RemoveReference = typename TRemoveReference<T>::_;
}

template<typename T> constexpr forceinline D::RemoveReference<T>&& Move(T&& t) noexcept {return static_cast<D::RemoveReference<T>&&>(t);}
template<typename T> struct TIsLValueReference: FalseType {};
template<typename T> struct TIsLValueReference<T&>: TrueType {};

template<typename T> constexpr forceinline T&& Forward(D::RemoveReference<T>& t) noexcept
{return static_cast<T&&>(t);}

template<typename T> constexpr forceinline T&& Forward(D::RemoveReference<T>&& t) noexcept
{
	static_assert(!TIsLValueReference<T>::_, "Bad Forward call!");
	return static_cast<T&&>(t);
}

template<typename T> forceinline void Swap(T&& a, T&& b)
{
	auto temp = Move(a);
	a = Move(b);
	b = Move(temp);
}

}

INTRA_WARNING_POP

}

typedef Intra::intptr ptrdiff_t;
typedef Intra::uintptr size_t;
