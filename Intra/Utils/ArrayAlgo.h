#pragma once

#include "Cpp/PlatformDetect.h"
#include "Cpp/Intrinsics.h"
#include "Concepts/Array.h"

namespace Intra {

namespace Range {

template<typename R1, typename R2> inline Meta::EnableIf<
	Concepts::IsTrivCopyCompatibleArrayWith<R1, R2>::_,
bool> Equals(const R1& r1, const R2& r2)
{
	if(Concepts::LengthOf(r1) != Concepts::LengthOf(r2)) return false;
	return C::memcmp(Concepts::DataOf(r1), Concepts::DataOf(r2), Concepts::LengthOf(r1)*sizeof(*Concepts::DataOf(r1)))==0;
}

template<typename R1, typename R2,
	typename T1 = Concepts::ElementTypeOfArray<R1>,
	typename T2 = Concepts::ElementTypeOfArray<R2>>
Meta::EnableIf<
	(Meta::IsIntegralType<T1>::_ || Meta::IsCharType<T1>::_) &&
	Meta::TypeEquals<T1, T2>::_ &&
	Concepts::IsArrayClass<R1>::_ &&
	Concepts::IsArrayClass<R2>::_ &&
	(sizeof(T1)==1 || INTRA_PLATFORM_ENDIANESS==INTRA_PLATFORM_ENDIANESS_BigEndian),
int> LexCompare(const R1& r1, const R2& r2)
{
	const size_t l1 = Concepts::LengthOf(r1), l2 = Concepts::LengthOf(r2);
	const int result = C::memcmp(Concepts::DataOf(r1), Concepts::DataOf(r2), (l1<l2? l1: l2)*sizeof(T1));
	if(result != 0) return result;
	if(l1 < l2) return -1;
	if(l1 > l2) return 1;
	return 0;
}

}

namespace Utils {

using Range::Equals;
using Range::LexCompare;

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
size_t> CStringLength(const dchar* str)
{return C::wcslen(reinterpret_cast<const wchar_t*>(str));}

template<typename U=dchar> forceinline Meta::EnableIf<
	sizeof(U)!=sizeof(wchar_t),
size_t> CStringLength(const dchar* str)
{
	const dchar* ptr = str-1;
	while(*++ptr!=0) {}
	return size_t(ptr-str);
}

}}
