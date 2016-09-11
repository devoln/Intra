#pragma once

#include "Core/Core.h"

namespace Intra {

template<typename T> struct AnotherEndian
{
	AnotherEndian() = default;
	AnotherEndian(T rhs) {operator=(rhs);}
	AnotherEndian(const AnotherEndian& rhs) = default;
	AnotherEndian& operator=(const AnotherEndian& rhs) = default;

	AnotherEndian& operator=(T rhs) {swap_bytes(bytes, (byte*)&rhs); return *this;}
	AnotherEndian& operator+=(T rhs) {return operator=(*this+rhs);}
	AnotherEndian& operator-=(T rhs) {return operator=(*this-rhs);}
	AnotherEndian& operator*=(T rhs) {return operator=(*this*rhs);}
	AnotherEndian& operator/=(T rhs) {return operator=(*this/rhs);}
	AnotherEndian& operator%=(T rhs) {return operator=(*this%rhs);}
	AnotherEndian& operator&=(T rhs) {return operator=(*this&rhs);}
	AnotherEndian& operator|=(T rhs) {return operator=(*this|rhs);}
	AnotherEndian& operator^=(T rhs) {return operator=(*this^rhs);}

	operator T() const {T result; swap_bytes((byte*)&result, bytes); return result;}
	template<typename U> operator U() const {return static_cast<U>(operator T());}

private:
	static void swap_bytes(byte* dst, const byte* src)
	{
		for(uint i=0; i<sizeof(T); i++)
			dst[i]=src[sizeof(T)-1-i];
	}

	byte bytes[sizeof(T)];
};

static_assert(Meta::IsAlmostPod<AnotherEndian<int>>::_, "AnotherEndian must be POD.");

template<typename T, typename U> auto operator+(U lhs, AnotherEndian<T> rhs) -> decltype(lhs+(T)rhs) {return lhs+(T)rhs;}
template<typename T, typename U> auto operator+(AnotherEndian<T> lhs, U rhs) -> decltype((T)lhs+rhs) {return (T)lhs+rhs;}
template<typename T, typename U> auto operator-(U lhs, AnotherEndian<T> rhs) -> decltype(lhs-(T)rhs) {return lhs-(T)rhs;}
template<typename T, typename U> auto operator-(AnotherEndian<T> lhs, U rhs) -> decltype((T)lhs-rhs) {return (T)lhs-rhs;}
template<typename T, typename U> auto operator*(U lhs, AnotherEndian<T> rhs) -> decltype(lhs*(T)rhs) {return lhs*(T)rhs;}
template<typename T, typename U> auto operator*(AnotherEndian<T> lhs, U rhs) -> decltype((T)lhs*rhs) {return (T)lhs*rhs;}
template<typename T, typename U> auto operator/(U lhs, AnotherEndian<T> rhs) -> decltype(lhs/(T)rhs) {return lhs/(T)rhs;}
template<typename T, typename U> auto operator/(AnotherEndian<T> lhs, U rhs) -> decltype((T)lhs/rhs) {return (T)lhs/rhs;}

template<typename T, typename U> auto operator^(U lhs, AnotherEndian<T> rhs) -> decltype(lhs^(T)rhs) {return lhs^(T)rhs;}
template<typename T, typename U> auto operator^(AnotherEndian<T> lhs, U rhs) -> decltype((T)lhs^rhs) {return (T)lhs^rhs;}
template<typename T, typename U> auto operator&(U lhs, AnotherEndian<T> rhs) -> decltype(lhs&(T)rhs) {return lhs&(T)rhs;}
template<typename T, typename U> auto operator&(AnotherEndian<T> lhs, U rhs) -> decltype((T)lhs&rhs) {return (T)lhs&rhs;}
template<typename T, typename U> auto operator|(U lhs, AnotherEndian<T> rhs) -> decltype(lhs|(T)rhs) {return lhs|(T)rhs;}
template<typename T, typename U> auto operator|(AnotherEndian<T> lhs, U rhs) -> decltype((T)lhs|rhs) {return (T)lhs|rhs;}

template<typename T, typename U> bool operator<(U lhs, AnotherEndian<T> rhs) {return lhs<(T)rhs;}
template<typename T, typename U> bool operator<(AnotherEndian<T> lhs, U rhs) {return (T)lhs<rhs;}
template<typename T, typename U> bool operator>(U lhs, AnotherEndian<T> rhs) {return lhs>(T)rhs;}
template<typename T, typename U> bool operator>(AnotherEndian<T> lhs, U rhs) {return (T)lhs>rhs;}
template<typename T, typename U> bool operator<=(U lhs, AnotherEndian<T> rhs) {return lhs<=(T)rhs;}
template<typename T, typename U> bool operator<=(AnotherEndian<T> lhs, U rhs) {return (T)lhs<=rhs;}
template<typename T, typename U> bool operator>=(U lhs, AnotherEndian<T> rhs) {return lhs>=(T)rhs;}
template<typename T, typename U> bool operator>=(AnotherEndian<T> lhs, U rhs) {return (T)lhs>=rhs;}
template<typename T, typename U> bool operator==(U lhs, AnotherEndian<T> rhs) {return lhs==(T)rhs;}
template<typename T, typename U> bool operator==(AnotherEndian<T> lhs, U rhs) {return (T)lhs==rhs;}
template<typename T, typename U> bool operator!=(U lhs, AnotherEndian<T> rhs) {return lhs!=(T)rhs;}
template<typename T, typename U> bool operator!=(AnotherEndian<T> lhs, U rhs) {return (T)lhs!=rhs;}





#if(INTRA_PLATFORM_ENDIANESS==INTRA_PLATFORM_ENDIANESS_LittleEndian)
typedef AnotherEndian<short> shortBE;
typedef AnotherEndian<ushort> ushortBE;
typedef AnotherEndian<int> intBE;
typedef AnotherEndian<uint> uintBE;
typedef AnotherEndian<long64> long64BE;
typedef AnotherEndian<ulong64> ulong64BE;

typedef short shortLE;
typedef ushort ushortLE;
typedef int intLE;
typedef uint uintLE;
typedef long64 long64LE;
typedef ulong64 ulong64LE;
#else
typedef AnotherEndian<short> shortLE;
typedef AnotherEndian<ushort> ushortLE;
typedef AnotherEndian<int> intLE;
typedef AnotherEndian<uint> uintLE;
typedef AnotherEndian<long64> long64LE;
typedef AnotherEndian<ulong64> ulong64LE;

typedef short shortBE;
typedef ushort ushortBE;
typedef int intBE;
typedef uint uintBE;
typedef long64 long64BE;
typedef ulong64 ulong64BE;
#endif

}

