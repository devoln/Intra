#pragma once

#include "IntraX/Core.h"
#include "Intra/Type.h"
#include "Intra/Misc/RawMemory.h"

INTRA_BEGIN
template<typename T> struct ReverseByteOrder
{
	constexpr ReverseByteOrder() = default;
	constexpr ReverseByteOrder(T rhs) {operator=(rhs);}

	constexpr ReverseByteOrder& operator=(T rhs) {ByteSwappedValue = Misc::ByteSwap(ByteSwappedValue); return *this;}
	constexpr ReverseByteOrder& operator+=(T rhs) {return operator=(*this+rhs);}
	constexpr ReverseByteOrder& operator-=(T rhs) {return operator=(*this-rhs);}
	constexpr ReverseByteOrder& operator*=(T rhs) {return operator=(*this*rhs);}
	constexpr ReverseByteOrder& operator/=(T rhs) {return operator=(*this/rhs);}
	constexpr ReverseByteOrder& operator%=(T rhs) {return operator=(*this%rhs);}
	constexpr ReverseByteOrder& operator&=(T rhs) {return operator=(*this&rhs);}
	constexpr ReverseByteOrder& operator|=(T rhs) {return operator=(*this|rhs);}
	constexpr ReverseByteOrder& operator^=(T rhs) {return operator=(*this^rhs);}

	constexpr operator T() const {return Misc::SwapByteOrder(result.mBytes, mBytes);}
	template<typename U> constexpr operator U() const {return Misc::SwapByteOrder(result.mBytes, mBytes);}

	T ByteSwappedValue;
};

static_assert(CTriviallySerializable<AnotherEndian<int>>);
template<typename T> using LittleEndian = TSelect<AnotherEndian<T>, T, Config::TargetIsBigEndian>;
template<typename T> using BigEndian = TSelect<AnotherEndian<T>, T, !Config::TargetIsBigEndian>;

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
INTRA_END
