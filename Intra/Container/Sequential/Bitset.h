#pragma once

#include "Core/Type.h"
#include "Core/Range/Span.h"
#include "Memory/Memory.h"
#include "Container/Sequential/Array.h"

//TODO
#if INTRA_DISABLED
INTRA_BEGIN

namespace Container {


class Bitset
{
	Array<uint64> v;

	enum: size_t {
		indexShift = 6,
		indexMask = 63
	};

	template<typename T, size_t BitsPerItem> void copyFrom(Span<T> arr)
	{
		if(BitsPerItem == sizeof(T)*8 && INTRA_PLATFORM_ENDIANESS == INTRA_PLATFORM_ENDIANESS_LittleEndian || sizeof(T) == 8)
		{
			const size_t dstBytes = v.Length()*sizeof(uint64);
			const size_t srcBytes = v.Length()*sizeof(T);
			const size_t minBytes = dstBytes < srcBytes? dstBytes: srcBytes;
			C::memcpy(v.Data(), arr.Data(), minBytes);
			C::memcpy(reinterpret_cast<char*>(v.Data()) + minBytes, reinterpret_cast<const char*>(arr.Data()) + minBytes, dstBytes - minBytes);
			return;
		}
		static_assert(sizeof(uint64) >= sizeof(T), "Precodition failed!");
		size_t arrWords = size_t(uint64(arr.Length()) * BitsPerItem / 64);
		const size_t vlen = v.Length();
		size_t n = vlen < arrWords? vlen: arrWords;
		for(size_t i = 0; i < n; i++)
		{
			uint64 x = 0;
			for(size_t bitOffset = 0; bitOffset < sizeof(uint64)*8; bitOffset += BitsPerItem)
			{
				x |= uint64(arr.First()) << bitOffset;
				arr.PopFirst();
			}
			v[i] = x;
		}
		if(vlen < arrWords || arr.Empty()) return;
		uint64 x = 0;
		for(size_t bitOffset = 0; !arr.Empty(); bitOffset += BitsPerItem)
		{
			x |= uint64(arr.First()) << bitOffset;
			arr.PopFirst();
		}
		v[arrWords] = x;
		for(size_t i = arrWords + 1; i < vlen; i++) v[i] = 0; //TODO: memset
	}
public:
	forceinline Bitset(null_t=null) {}

	template<typename T, typename=Requires<
		CIntegral<TRemoveConstRef<T>>::_
	>> forceinline Bitset(Span<T> arr)
	{copyFrom<T, sizeof(T)*8>(arr);}

	forceinline Bitset(CSpan<bool> arr)
	{copyFrom<const bool, 1>(arr);}

	//! Initialize bitset using a list of 64-bit numbers.
	/*!
	  If it is longer than necessary redundant bits of initializer will be discarded.
	  If it is shorter than the bitset length remaining bits of constructed bitset will be zero initialized.
	  By performance reasons when possible prefer using a single argument version of constructor defined below:
	  SBitset(uint64Value) instead of SBitset{uint64Value}
	*/
	forceinline Bitset(InitializerList<uint64> arr)
	{copyFrom<const uint64, sizeof(uint64)*8>(arr);}

	//! Initialize only the first 64 bits.
	/*!
	  If N > 64 the remaining bits will be zero.
	  Use SBitset(x) when possible because the compiler will more likely optimize this rather than initializer list version (SBitset{x}).
	  Anyway there is no performance difference with -O2 or higher in GCC, Clang and MSVC.
	*/
	forceinline Bitset(uint64 x): v{uint64(x)} {}

	Bitset(const Bitset&) = default;
	Bitset(Bitset&&) = default;

	//! @returns true if flag at \p index is set.
	//! @pre index must be less than N
	forceinline bool operator[](size_t index) const
	{
		INTRA_DEBUG_ASSERT(index < N);
		return ((v[index >> indexShift] >> (index & indexMask)) & 1) != 0;
	}

	//! @returns true if index < N and flag at \p index is set.
	/*!
	  Similar to operator[] but this version allows Bitset to be used as a predicate.
	*/
	template<typename Char> constexpr forceinline bool operator()(size_t index) const {return index < N && operator[](index);}

	forceinline void Set(size_t index)
	{
		INTRA_DEBUG_ASSERT(index < N);
		v[index >> indexShift] |= uint64(uint64(1) << (index & indexMask));
	}

	forceinline void Reset(size_t index)
	{
		INTRA_DEBUG_ASSERT(index < N);
		v[index >> indexShift] &= uint64(~(uint64(1) << (index & indexMask)));
	}

	//! Set all bits to zero.
	INTRA_CONSTEXPR2 Bitset& operator=(null_t) noexcept
	{
		Memory::Initialize(v.AsRange());
		return *this;
	}

	//! Check if all bits are zero.
	bool operator==(null_t) const noexcept
	{
		const size_t n = v.Length();
		const auto data = v.Data();

		// Most compilers vectorize this loop
		uint64 res = 0;
		for(size_t i = 0; i < n; i++)
			res |= data[i];
		return res == 0;
		// TODO: check if it is at least 2x faster than naive implementation with early exit:
#if INTRA_DISABLED
		for(size_t i = 0; i < n; i++)
			if(data[i]) return false;
		return true;
#endif
	}

	forceinline bool operator!=(null_t) const noexcept {return !operator==(null);}

	/*! Check if two bitsets are equal.
	  Two bitsets are considered equal if they have same Length() and all their corresponding bits are equal.
	*/
	bool operator==(const Bitset& rhs) const noexcept
	{
		const size_t n = v.Length();
		if(n != rhs.v.Length()) return false;
		const auto data = v.Data();
		const auto rhsData = rhs.v.Data();

		// Most compilers vectorize this loop
		uint64 res = 0;
		for(size_t i = 0; i < n; i++)
			res |= data[i] ^ rhsData[i];
		return res == 0;
		// TODO: check if it is at least 2x faster than naive implementation with early exit:
#if INTRA_DISABLED
		for(size_t i = 0; i < n; i++)
			if(data[i] != rhsData[i]) return false;
		return true;
#endif
	}

	forceinline bool operator!=(const Bitset& rhs) const noexcept {return !operator==(rhs);}

	//! Set union
	forceinline Bitset operator|=(const Bitset& rhs) noexcept
	{
		const size_t n = v.Length();
		if(n > rhs.v.Length()) n = rhs.v.Length();
		for(size_t i = 0; i < n; i++)
			v[i] |= rhs.v[i];
		return *this;
	}

	//! Set insersection
	INTRA_CONSTEXPR2 Bitset operator&=(const Bitset& rhs) noexcept
	{
		for(size_t i = 0; i < nWords; i++)
			v[i] &= rhs.v[i];
		return *this;
	}

	//! Set negation
	INTRA_CONSTEXPR2 void Negate() noexcept
	{
		for(size_t i = 0; i < nWords; i++)
			v[i] = ~v[i];
	}

	//! Set union
	INTRA_CONSTEXPR2 Bitset operator|(Bitset rhs) const noexcept
	{return rhs |= *this;}

	//! Set intersection
	INTRA_CONSTEXPR2 Bitset operator&(Bitset rhs) const noexcept
	{return rhs &= *this;}

	//! Set negation
	INTRA_CONSTEXPR2 SBitset operator~() const noexcept
	{
		SBitset result;
		for(size_t i = 0; i < nWords; i++) result.v[i] = uint64(~v[i]);
		result.v[nWords-1] &= remainderMask;
		return result;
	}

	//! Check if this bitset contains \p subset.
	//! This allows SBitset to be used as a predicate.
	INTRA_CONSTEXPR2 bool operator()(const SBitset& subset) const
	{
		uint64 res = 0;
		for(size_t i = 0; i < nWords; i++)
			res |= (v[i] & subset.v[i]) ^ subset.v[i];
		return res == 0;
	}

	constexpr forceinline explicit operator uint64() const {return v[0];}
};

}
INTRA_END

#endif
