#include "Ascii.h"
#include <Intra/Range.h>
#include <Intra/Range/Comparison.h>
#include <Intra/Range/Search/Single.h>
#include <Intra/Range/Mutation/ReplaceSubrange.h>
#include <Intra//Range/StringView.h>
#include <Intra/Container/AsciiSet.h>
#include <Intra/Simd/Simd.h>

// TODO: make this a particular case of existing generic algorithms

namespace Intra { INTRA_BEGIN
void StringFindAscii(StringView& str, Span<const StringView> stopSubStrSet, size_t* oWhichIndex)
{
	if(stopSubStrSet.Empty())
	{
		TailAdvance(str, 0);
		if(oWhichIndex) *oWhichIndex = 0;
		return;
	}
	AsciiSet firstChars;
	for(size_t i = 0; i<stopSubStrSet.Length(); i++)
	{
		if(stopSubStrSet[i].Empty())
		{
			if(oWhichIndex) *oWhichIndex = i;
			return;
		}
		firstChars.Set(stopSubStrSet[i].First());
	}
	for(;;)
	{
		FindAdvance(str, firstChars);
		if(str.Empty())
		{
			if(oWhichIndex) *oWhichIndex = stopSubStrSet.Length();
			return;
		}
		str.PopFirst();
		for(size_t i=0; i<stopSubStrSet.Length(); i++)
		{
			if(!StartsWith(str, stopSubStrSet[i].Drop())) continue;
			if(oWhichIndex) *oWhichIndex = i;
			return;
		}
	}
}

INTRA_FORCEINLINE StringView StringReadUntilAscii(StringView& str, Span<const StringView> stopSubStrSet, size_t* oWhichIndex)
{
	const char* begin = str.Data();
	StringFindAscii(str, stopSubStrSet, oWhichIndex);
	return StringView::FromPointerRange(begin, str.Data());
}

size_t StringMultiReplaceAsciiLength(StringView src,
	Span<const StringView> fromSubStrs, Span<const StringView> toSubStrs)
{
	INTRA_PRECONDITION(fromSubStrs.Length() == toSubStrs.Length());
	size_t substrIndex = 0;
	size_t len = 0;
	while(len += StringReadUntilAscii(src, fromSubStrs, &substrIndex).Length(), !src.Empty())
	{
		len += toSubStrs[substrIndex].Length();
		src.PopFirstExactly(fromSubStrs[substrIndex].Length());
	}
	return len;
}

StringView StringMultiReplaceAscii(StringView src, GenericStringView<char>& dstBuffer,
	Span<const StringView> fromSubStrs, Span<const StringView> toSubStrs)
{
	INTRA_PRECONDITION(fromSubStrs.Length() == toSubStrs.Length());
	char* begin = dstBuffer.Data();
	size_t substrIndex = 0;
	while(WriteTo(StringReadUntilAscii(src, fromSubStrs, &substrIndex), dstBuffer), !src.Empty())
	{
		WriteTo(toSubStrs[substrIndex], dstBuffer);
		src.PopFirstExactly(fromSubStrs[substrIndex].Length());
	}
	return StringView::FromPointerRange(begin, dstBuffer.Data());
}

INTRA_FORCEINLINE bool getBit(int index, const uint32 bitset[])
{ // choose a combination to make most compilers emit bt instruction which is the fastest on x86 (still couldn't achieve this on 32-bit MSVC)
#if (defined(__GNUC__) || defined(__clang__)) && defined(__i386__) || \
	defined(__x86_64__) || defined(_MSC_VER) && defined(_M_IX86) || defined(__arm__) || defined(__aarch64__)
	return (bitset[c >> 5] >> (c & 31)) & 1;
#elif defined(__i386__) || defined(__x86_64__) || defined(_M_X64) 
	return bitset[c >> 5] & (1 << c);
#else
	return bitset[c >> 5] & (1 << (c & 31));
#endif
}

#if defined(_MSC_VER) && !defined(__clang__) && defined(_M_IX86)
__declspec(naked) int __fastcall MemoryFindAscii32m(const uint32 charCodeBitset[8], const char* str, int n)
{
	__asm {
		push    edi
		push    ebx
		push    esi
		mov     ebx, DWORD PTR [esp+16]
		xor     eax, eax
		test    ebx, ebx
		je      L17
	L18:
		movzx   edi, BYTE PTR [edx+eax]
		mov     esi, edi
		sar     edi, 5
		mov     edi, DWORD PTR [ecx+edi*4]
		bt      edi, esi
		jc      L17

		add     eax, 1
		cmp     eax, ebx
		jne     L18
	L17:
		pop     esi
		pop     ebx
		pop     edi
		ret     4
	}
}
#else
INTRA_NOINLINE int INTRA_FASTCALL MemoryFindAscii32m(uint32 asciiSet[4], const char* str, int n)
{
	for(int i = 0; i < n; i++)
	{
		int c = str[i];
		if(!getBit(c, asciiSet)) continue;
		return i;
	}
	return n;
}
#endif

#if defined(__i386__) || defined(__amd64__)
INTRA_TARGET_SSSE3 INTRA_NOINLINE int __fastcall MemoryFindAscii128(const uint32 asciiSet[4], const char* str, int n)
{
	INTRA_PRECONDITION(IsAligned(n, 16));
	using Vec = SimdVector<uint8, 16>;
	const Vec mask7 = SimdVectorFilled<uint8, 16>(7);
	const Vec mask15 = SimdVectorFilled<uint8, 16>(0x0F);
	const Vec bit7 = SimdVectorFilled<uint8, 16>(0x80);
	const Vec bits = {1, 2, 4, 8, 16, 32, 64, 128, 0, 0, 0, 0, 0, 0, 0, 0};
	const Vec zero = {};
	const Vec asciiSetR = Vec(SimdLoad<4>(asciiSet));
	for(int i = 0; i < n; i += 16)
	{
		const auto cx = Vec(SimdLoad<16>(&str[i]));
		auto byteShuffleMask = Vec(SimdVector<uint16, 8>(cx) >> 3) & mask15;
		byteShuffleMask |= cx & bit7; // handle non-ASCII chars
		const Vec asciiSetBytesForCx = SimdShuffleDynamic(asciiSetR, byteShuffleMask);
		const Vec bitsToTest = SimdShuffleDynamic(bits, cx & mask7);
		const Vec resMask = (asciiSetBytesForCx & bitsToTest) == zero;
		const auto res = SimdMoveByteMask(resMask);
		if(res != 0xFFFF) return i + CountTrailingZeros<uint16>(uint16(~res));
	}
	return n;
}
#endif

#ifdef __ARM_NEON
#ifdef __aarch64__
INTRA_NOINLINE int INTRA_FASTCALL MemoryFindAscii128(unsigned asciiSet[4], const char* str, int n)
{
	const auto asciiSetR = SimdVector<uint8, 16>(SimdLoad<4>(asciiSet));
	for(int i = 0; i < n; i += 16)
	{
		const auto cx = SimdVector<uint8, 16>(SimdLoad<16>(&str[i]));
		const auto lookupRes = vqtbl1q_u8(asciiSetR, cx >> 3) & (1 << (cx & 7));
		if(const auto low = uint64(vget_low_u32(SimdVector<uint32, 4>(lookupRes))))
			return i + (CountTrailingZeros<uint64>(low) >> 3);
		if(const auto high = uint64(vget_high_u32(SimdVector<uint32, 4>(lookupRes))))
			return i + 8 + (CountTrailingZeros<uint64>(high) >> 3);
	}
	return n;
}
#else
INTRA_NOINLINE int INTRA_FASTCALL MemoryFindAscii128(unsigned asciiSet[4], const char* str, int n)
{
	const auto asciiSetR = SimdVector<uint8, 16>(SimdLoad<4>(asciiSet)); // uint8x8x2
	for(int i = 0; i < n; i += 8)
	{
		const auto cx = SimdVector<uint8, 8>(SimdLoad<8>(&str[i]));
		const auto lookupRes = uint64(vtbl2_u8(asciiSetR, cx >> 3) & (1 << (cx & 7)));
		if(lookupRes) return i + (CountTrailingZeros<uint64>(lookupRes) >> 3);
	}
	return n;
}
#endif
#endif

} INTRA_END
