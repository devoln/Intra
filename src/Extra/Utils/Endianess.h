#pragma once

#include "Extra/Core.h"
#include "Intra/Type.h"

EXTRA_BEGIN
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
		for(unsigned i=0; i<sizeof(T); i++)
			dst[i] = src[sizeof(T)-1-i];
	}

public:
	union
	{
		T mValue;
		byte mBytes[sizeof(T)];
	};
};

static_assert(CTriviallySerializable<AnotherEndian<int>>);
template<typename T> using LittleEndian = TSelect<AnotherEndian<T>, T, TargetIsBigEndian>;
template<typename T> using BigEndian = TSelect<AnotherEndian<T>, T, !TargetIsBigEndian>;

using int16BE = BigEndian<int16>;
using uint16BE = BigEndian<uint16>;
using int32BE = BigEndian<int32>;
using uint32BE = BigEndian<uint32>;
using int64BE = BigEndian<int64>;
using uint64BE = BigEndian<uint64>;

using int16LE = LittleEndian<int16>;
using ushortLE = LittleEndian<uint16>;
using int32LE = LittleEndian<int32>;
using uint32LE = LittleEndian<uint32>;
using int64LE = LittleEndian<int64>;
using uint64LE = LittleEndian<uint64>;
EXTRA_END
