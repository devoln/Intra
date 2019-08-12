#pragma once

#include "Core/Type.h"

INTRA_BEGIN

namespace Utils {

template<typename T> struct AnotherEndian
{
	AnotherEndian() = default;
	AnotherEndian(T rhs) {operator=(rhs);}

	AnotherEndian& operator=(T rhs) {swap_bytes(mBytes, reinterpret_cast<byte*>(&rhs)); return *this;}
	AnotherEndian& operator+=(T rhs) {return operator=(*this+rhs);}
	AnotherEndian& operator-=(T rhs) {return operator=(*this-rhs);}
	AnotherEndian& operator*=(T rhs) {return operator=(*this*rhs);}
	AnotherEndian& operator/=(T rhs) {return operator=(*this/rhs);}
	AnotherEndian& operator%=(T rhs) {return operator=(*this%rhs);}
	AnotherEndian& operator&=(T rhs) {return operator=(*this&rhs);}
	AnotherEndian& operator|=(T rhs) {return operator=(*this|rhs);}
	AnotherEndian& operator^=(T rhs) {return operator=(*this^rhs);}

	operator T() const {AnotherEndian result; swap_bytes(result.mBytes, mBytes); return result.mValue;}
	template<typename U> operator U() const {return static_cast<U>(operator T());}

private:
	static void swap_bytes(byte* dst, const byte* src)
	{
		for(uint i=0; i<sizeof(T); i++)
			dst[i] = src[sizeof(T)-1-i];
	}

	union
	{
		T mValue;
		byte mBytes[sizeof(T)];
	};
};

static_assert(CPod<AnotherEndian<int>>, "AnotherEndian must be POD.");


#if(INTRA_PLATFORM_ENDIANESS == INTRA_PLATFORM_ENDIANESS_LittleEndian)
typedef AnotherEndian<short> shortBE;
typedef AnotherEndian<ushort> ushortBE;
typedef AnotherEndian<int> intBE;
typedef AnotherEndian<uint> uintBE;
typedef AnotherEndian<int64> long64BE;
typedef AnotherEndian<uint64> ulong64BE;

typedef short shortLE;
typedef ushort ushortLE;
typedef int intLE;
typedef uint uintLE;
typedef int64 long64LE;
typedef uint64 ulong64LE;
#else
typedef AnotherEndian<short> shortLE;
typedef AnotherEndian<ushort> ushortLE;
typedef AnotherEndian<int> intLE;
typedef AnotherEndian<uint> uintLE;
typedef AnotherEndian<int64> long64LE;
typedef AnotherEndian<uint64> ulong64LE;

typedef short shortBE;
typedef ushort ushortBE;
typedef int intBE;
typedef uint uintBE;
typedef int64 long64BE;
typedef uint64 ulong64BE;
#endif

}
INTRA_END
