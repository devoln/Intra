#pragma once

#include "Core/Assert.h"
#include "Core/Numeric.h"

INTRA_BEGIN
/** Class to represent set of bits or flags of specified compile-time length.
  Implementation note:
  The code is written to support constexpr operation in C++14 and above and to support vectorization capabilities of compiler runtime optimizations.
  GCC and Clang optimize it perfectly with -O2 and -O3 (and -O1 for GCC) flags but not -Os (and -O1 for Clang).
  Perfectly means without any overhead over manually written bit manipulation code.
  MSVC optimizes it well but there is some overhead with any optimization option.
*/
template<size_t N> struct SBitset
{
private:
	enum {
		nBytes = (N + 7) / 8,
		nBytesPerWord = (nBytes < 8)? nBytes: 8
	};
	typedef TIntMin<nBytesPerWord> basicType;
	enum: size_t {
		nWords = (nBytes + sizeof(basicType) - 1) / sizeof(basicType),
		indexShift =
			sizeof(basicType) == 8? 6:
			sizeof(basicType) == 4? 5:
			sizeof(basicType) == 2? 4:
			sizeof(basicType) == 1? 3:
			-1,
		indexMask = sizeof(basicType)*8-1,
		remainderBits = N & indexMask
	};
	enum: basicType {
		remainderMask = basicType(~basicType()) >> basicType(remainderBits == 0? 0: (sizeof(basicType)*8 - remainderBits))
	};
	basicType v[nWords];

	template<typename T, size_t BitsPerItem> constexpr void copyFrom(Span<T> arr)
	{
		static_assert(sizeof(basicType) >= sizeof(T) || nBytes <= sizeof(T), "Precodition failed!");
		size_t arrWords = arr.Length() * BitsPerItem / (sizeof(basicType) * 8);
		size_t n = nWords < arrWords? nWords: arrWords;
		for(size_t i = 0; i < n; i++)
		{
			basicType x = 0;
			for(size_t bitOffset = 0; bitOffset < sizeof(basicType)*8; bitOffset += BitsPerItem)
			{
				x |= basicType(arr.First()) << bitOffset;
				arr.PopFirst();
			}
			v[i] = x;
		}
		if(nWords < arrWords || arr.Empty()) return;
		basicType x = 0;
		for(size_t bitOffset = 0; !arr.Empty(); bitOffset += BitsPerItem)
		{
			x |= basicType(arr.First()) << bitOffset;
			arr.PopFirst();
		}
		v[arrWords] = x;
		//They are already zeros in constructor
		//for(size_t i = arrWords + 1; i < nWords; i++) v[i] = 0;
	}
public:
	constexpr forceinline SBitset(null_t=null): v{} {}

	template<typename T, typename=Requires<
		CIntegral<TRemoveConstRef<T>>
	>> constexpr forceinline SBitset(Span<T> arr): v{}
	{copyFrom<T, sizeof(T)*8>(arr);}

	constexpr forceinline SBitset(CSpan<bool> arr): v{}
	{copyFrom<const bool, 1>(arr);}

	//! Initialize bitset using a list of 64-bit numbers.
	/*!
	  If it is longer than necessary redundant bits of initializer will be discarded.
	  If it is shorter than the bitset length remaining bits of constructed bitset will be zero initialized.
	  By performance reasons when possible prefer using a single argument version of constructor defined below:
	  SBitset(uint64Value) instead of SBitset{uint64Value}
	*/
	constexpr forceinline SBitset(InitializerList<uint64> arr): v{}
	{copyFrom<const uint64, sizeof(uint64)*8>(arr);}

	//! Initialize only the first 64 bits.
	/*!
	  If N > 64 the remaining bits will be zero.
	  Use SBitset(x) when possible because the compiler will more likely optimize this rather than initializer list version (SBitset{x}).
	  Anyway there is no performance difference with -O2 or higher in GCC, Clang and MSVC.
	*/
	constexpr forceinline SBitset(uint64 x): v{basicType(x)} {}

	constexpr SBitset(const SBitset&) = default;

	//! @returns true if flag at \p index is set.
	//! @pre index must be less than N
	constexpr forceinline bool operator[](size_t index) const
	{
		return INTRA_DEBUG_ASSERT(index < N),
			((v[index >> indexShift] >> (index & indexMask)) & 1) != 0;
	}

	//! @returns true if index < N and flag at \p index is set.
	/*!
	  Similar to operator[] but this version allows SBitset to be used as a predicate.
	*/
	template<typename Char> constexpr forceinline bool operator()(size_t index) const {return index < N && operator[](index);}

	constexpr forceinline void Set(size_t index)
	{
		INTRA_PRECONDITION(index < N);
		v[index >> indexShift] |= basicType(basicType(1) << (index & indexMask));
	}

	constexpr forceinline void Reset(size_t index)
	{
		INTRA_PRECONDITION(index < N);
		v[index >> indexShift] &= basicType(~(basicType(1) << (index & indexMask)));
	}

	//! set all bits to zero.
	constexpr SBitset& operator=(null_t) noexcept
	{
		for(size_t i = 0; i < nWords; i++)
			v[i] = 0;
		return *this;
	}

	//! Check if all bits are zero.
	constexpr bool operator==(null_t) const noexcept
	{
		basicType res = 0;
		for(size_t i = 0; i < nWords; i++)
			res |= v[i];
		return res == 0;
	}

	constexpr forceinline bool operator!=(null_t) const noexcept {return !operator==(null);}

	constexpr bool operator==(const SBitset& rhs) const noexcept
	{
		basicType res = 0;
		for(size_t i = 0; i < nWords; i++)
			res |= v[i] ^ rhs.v[i];
		return res == 0;
	}

	constexpr forceinline bool operator!=(const SBitset& rhs) const noexcept {return !operator==(rhs);}

	//! Set union
	constexpr SBitset operator|=(const SBitset& rhs) noexcept
	{
		for(size_t i = 0; i < nWords; i++)
			v[i] |= rhs.v[i];
		return *this;
	}

	//! Set insersection
	constexpr SBitset operator&=(const SBitset& rhs) noexcept
	{
		for(size_t i = 0; i < nWords; i++)
			v[i] &= rhs.v[i];
		return *this;
	}

	//! Set negation
	constexpr void Negate() noexcept
	{
		for(size_t i = 0; i < nWords; i++)
			v[i] = ~v[i];
	}

	//! Set union
	constexpr SBitset operator|(SBitset rhs) const noexcept
	{return rhs |= *this;}

	//! Set intersection
	constexpr SBitset operator&(SBitset rhs) const noexcept
	{return rhs &= *this;}

	//! Set negation
	constexpr SBitset operator~() const noexcept
	{
		SBitset result;
		for(size_t i = 0; i < nWords; i++) result.v[i] = basicType(~v[i]);
		result.v[nWords-1] &= remainderMask;
		return result;
	}

	//! Check if this bitset contains \p subset.
	//! This allows SBitset to be used as a predicate.
	constexpr bool operator()(const SBitset& subset) const
	{
		basicType res = 0;
		for(size_t i = 0; i < nWords; i++)
			res |= (v[i] & subset.v[i]) ^ subset.v[i];
		return res == 0;
	}

	constexpr forceinline explicit operator uint64() const {return v[0];}
};

#if INTRA_CONSTEXPR_TEST
static_assert(SBitset<8>{0xFA} != null, "TEST FAILED!");
static_assert((SBitset<16>(0xFAAD) & SBitset<16>{0xAA0}) == SBitset<16>{0xAA0}, "TEST FAILED!");
static_assert(SBitset<64>{0xFAADABCD09876543} == SBitset<64>(0xFAADABCD09876543), "TEST FAILED!");
static_assert((
	SBitset<67>{0x0000FAADA0A0A0A0, 3} |
	SBitset<67>{0xFAAD00000B0B0B0B, 6}) ==
	SBitset<67>{0xFAADFAADABABABAB, 7}, "TEST FAILED!");
static_assert(~SBitset<128>{0xAFAFAFAFAFAFAFAF, 0xBCBCBCBCBCBCBCBC} == SBitset<128>{0x5050505050505050, 0x4343434343434343}, "TEST FAILED!");
static_assert(~SBitset<3>(CSpan<bool>{false, false, true}) == SBitset<3>(CSpan<bool>{true, true, false}), "TEST FAILED!");
#endif

INTRA_END
