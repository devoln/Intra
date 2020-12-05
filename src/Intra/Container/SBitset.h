#pragma once

#include "Intra/Assert.h"
#include "Intra/Numeric.h"
#include "Intra/TypeSafe.h"
#include "Intra/Range/Operations.h"
#include "Misc/RawMemory.h"

INTRA_BEGIN
/** Class to represent a set of bits or flags of specified compile-time length.
*/
template<size_t N> class SBitset
{
	static constexpr size_t nBytes = (N + 7) / 8;
	static constexpr size_t nBytesPerWord = (nBytes < 8)? nBytes: 8;
	using basicType = TUnsignedIntMin<nBytesPerWord>;
	static constexpr size_t nWords = (nBytes + sizeof(basicType) - 1) / sizeof(basicType);
	static constexpr size_t indexShift =
		CSame<basicType, uint64>? 6:
		CSame<basicType, uint32>? 5:
		CSame<basicType, uint16>? 4:
		CSame<basicType, uint8>? 3:
		-1;
	static constexpr size_t indexMask = sizeof(basicType)*8-1;
	static constexpr size_t remainderBits = N & indexMask;
	static constexpr basicType remainderMask = MaxValueOf<basicType> >>
		basicType(remainderBits == 0? 0: (sizeof(basicType)*8 - remainderBits));

	basicType v[nWords]{};

	INTRA_OPTIMIZE_FUNCTION(template<typename R, size_t BitsPerItem>)
	constexpr void copyStartBitsFrom(R&& range)
	{
		using T = TRangeValue<R>;
		static_assert(sizeof(basicType) >= sizeof(T) || nBytes <= sizeof(T));
		size_t arrWholeWords = size_t(range.Length()) * BitsPerItem / (sizeof(basicType) * 8);
		for(size_t i = 0, n = Min(nWords, arrWholeWords); i < n; i++)
		{
			basicType x = 0;
			for(size_t bitOffset = 0; bitOffset < sizeof(basicType)*8; bitOffset += BitsPerItem)
				x |= basicType(range|Next) << bitOffset;
			v[i] = x;
		}
		if(nWords < arrWholeWords || range.Empty()) return;
		basicType leftOver = 0;
		for(size_t bitOffset = 0; !range.Empty(); bitOffset += BitsPerItem)
			leftOver |= basicType(range|Next) << bitOffset;
		v[arrWholeWords] = leftOver;
	}
	INTRA_OPTIMIZE_FUNCTION_END
public:
	constexpr SBitset() = default;

	template<CIntegral T> constexpr SBitset(Span<T> arr) {copyStartBitsFrom<T, sizeof(T)*8>(arr);}

	constexpr SBitset(CSpan<bool> arr) {copyStartBitsFrom<const bool, 1>(arr);}

	template<CLosslessConvertible<uint64>... UInt64s> requires (sizeof...(UInt64s) <= nWords)
	constexpr SBitset(UInt64s... x): v{basicType(x)...} {INTRA_PRECONDITION((x >= 0 && ...));}

	constexpr SBitset(const SBitset&) = default;

	/// @returns true if the flag at \p index is set.
	/// @pre index must be less than N
	constexpr bool operator[](Index index) const
	{
		INTRA_PRECONDITION(index < N);
		return ((v[size_t(index) >> indexShift] >> (size_t(index) & indexMask)) & 1) != 0;
	}

	/** @returns true if index < N and flag at \p index is set.

	  Similar to operator[] but this version allows SBitset to be used as a predicate.
	*/
	constexpr bool operator()(Index index) const {return operator[](index);}

	constexpr void Set(Index index)
	{
		INTRA_PRECONDITION(index < N);
		v[size_t(index) >> indexShift] |= basicType(basicType(1) << (size_t(index) & indexMask));
	}

	constexpr void Reset(Index index)
	{
		INTRA_PRECONDITION(size_t(index) < N);
		v[size_t(index) >> indexShift] &= basicType(~(basicType(1) << (size_t(index) & indexMask)));
	}

	/// Set all bits to zero.
	constexpr void ResetAllBits() noexcept {BitwiseZero(v, nWords);}

	/// Check if all bits are zero.
	constexpr bool AreAllBitsUnset() const noexcept
	{
		basicType res = 0;
		for(size_t i = 0; i < nWords; i++) res |= v[i];
		return res == 0;
	}

	constexpr bool operator==(const SBitset& rhs) const noexcept
	{
		basicType res = 0;
		for(size_t i = 0; i < nWords; i++) res |= v[i] ^ rhs.v[i];
		return res == 0;
	}

	constexpr bool operator!=(const SBitset& rhs) const noexcept {return !operator==(rhs);}

	/// Set union
	constexpr SBitset operator|=(const SBitset& rhs) noexcept
	{
		for(size_t i = 0; i < nWords; i++) v[i] |= rhs.v[i];
		return *this;
	}

	/// Set insersection
	constexpr SBitset operator&=(const SBitset& rhs) noexcept
	{
		for(size_t i = 0; i < nWords; i++) v[i] &= rhs.v[i];
		return *this;
	}

	/// Set negation
	constexpr void Negate() noexcept {for(auto& w: v) w = ~w;}

	/// Set union
	constexpr SBitset operator|(SBitset rhs) const noexcept {return rhs |= *this;}

	/// Set intersection
	constexpr SBitset operator&(SBitset rhs) const noexcept {return rhs &= *this;}

	/// Set negation
	constexpr SBitset operator~() const noexcept
	{
		SBitset result;
		for(size_t i = 0; i < nWords; i++) result.v[i] = basicType(~v[i]);
		result.v[nWords-1] &= remainderMask;
		return result;
	}

	/// Check if this bitset contains `subset`.
	/// This allows SBitset to be used as a predicate.
	constexpr bool operator()(const SBitset& subset) const
	{
		basicType res = 0;
		for(size_t i = 0; i < nWords; i++)
			res |= (v[i] & subset.v[i]) ^ subset.v[i];
		return res == 0;
	}

	template<CUnsignedIntegral T> requires(sizeof(T) >= N/8)
	constexpr explicit operator T() const noexcept {return v[0];}
};

#if INTRA_CONSTEXPR_TEST
static_assert(SBitset<8>{0xFA} != null);
static_assert((SBitset<16>(0xFAAD) & SBitset<16>{0xAA0}) == SBitset<16>{0xAA0});
static_assert(SBitset<64>{0xFAADABCD09876543} == SBitset<64>(0xFAADABCD09876543));
static_assert((
	SBitset<67>{0x0000FAADA0A0A0A0, 3} |
	SBitset<67>{0xFAAD00000B0B0B0B, 6}) ==
	SBitset<67>{0xFAADFAADABABABAB, 7});
static_assert(~SBitset<128>{0xAFAFAFAFAFAFAFAF, 0xBCBCBCBCBCBCBCBC} == SBitset<128>{0x5050505050505050, 0x4343434343434343});
static_assert(~SBitset<3>(CSpan<bool>{false, false, true}) == SBitset<3>(CSpan<bool>{true, true, false}));
#endif

INTRA_END
