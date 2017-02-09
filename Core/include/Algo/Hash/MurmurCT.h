#pragma once

//! В данном заголовочном файле определена compile-time версия MurmurHash 3.
//! Она также может работать в run-time, но скорее всего будет медленнее,
//! чем версия, определённая в файле Murmur.h

#include "Types.h"

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable: 4307)
INTRA_DISABLE_REDUNDANT_WARNINGS
#endif

namespace Intra { namespace Algo { namespace HashCT {

namespace D {

constexpr inline uint murmur3_32_k(uint k)
{
	return uint(((uint(k * 0xcc9e2d51ull) << 15)
		| (uint(k * 0xcc9e2d51ull) >> 17)) * 0x1b873593ull);
}

constexpr inline uint murmur3_32_hashround(uint k, uint hash)
{
	return uint(uint((((hash^k) << 13)
		| ((hash^k) >> 19)) * 5ull) + 0xe6546b64ull);
}

constexpr inline uint word32le(const char* s, int len=4)
{
	return (len>0? static_cast<uint>(s[0]): 0)
		+ (len>1? (static_cast<uint>(s[1]) << 8): 0)
		+ (len>2? (static_cast<uint>(s[2]) << 16): 0)
		+ (len>3? (static_cast<uint>(s[3]) << 24): 0);
}

constexpr inline uint murmur3_32_loop(const char* key, size_t len, uint hash)
{
	return len == 0 ? hash :
		murmur3_32_loop(
			key + 4,
			len - 1,
			murmur3_32_hashround(
				murmur3_32_k(word32le(key)), hash));
}


constexpr inline uint murmur3_32_end0(uint k)
{
	return uint( ( (uint(k*0xcc9e2d51ull) << 15)
		| (uint(k*0xcc9e2d51ull) >> 17) ) * 0x1b873593ull);
}

constexpr inline uint murmur3_32_end1(uint k, const char* key)
{
	return murmur3_32_end0(
		k ^ static_cast<uint>(key[0]));
}

constexpr inline uint murmur3_32_end2(uint k, const char* key)
{
	return murmur3_32_end1(
		k ^ (static_cast<uint>(key[1]) << 8), key);
}

constexpr inline uint murmur3_32_end3(uint k, const char* key)
{
	return murmur3_32_end2(
		k ^ (static_cast<uint>(key[2]) << 16), key);
}

constexpr inline uint murmur3_32_end(uint hash, const char* key, size_t rem)
{
	return rem == 0 ? hash :
		hash ^ (rem == 3 ? murmur3_32_end3(0, key):
			rem == 2 ? murmur3_32_end2(0, key):
			murmur3_32_end1(0, key));
}

constexpr inline uint murmur3_32_final1(uint hash)
{return uint((hash ^ (hash >> 16)) * 0x85ebca6bull);}

constexpr inline uint murmur3_32_final2(uint hash)
{return uint((hash ^ (hash >> 13)) * 0xc2b2ae35ull);}

constexpr inline uint murmur3_32_final3(uint hash)
{return hash ^ (hash >> 16);}

constexpr inline uint murmur3_32_final(uint hash, size_t len)
{
	return murmur3_32_final3(
			murmur3_32_final2(
				murmur3_32_final1(hash ^ static_cast<uint>(len))));
}

constexpr inline uint murmur3_32_value(const char* key, size_t len, uint seed)
{
	return murmur3_32_final(murmur3_32_end(
		murmur3_32_loop(key, len/4, seed),
		key+(len/4)*4, len&3), len);
}



//Murmur3 128-bit
enum: ulong64 {_c1=0x87c37b91114253d5ULL, _c2=0x4cf5ad432745937fULL};

constexpr inline byte getU8(const char* str, size_t n)
{return byte(str[n]);}

constexpr inline ulong64 getU64(const char* str, size_t n)
{
	return ulong64(byte(str[n*8]))         | ulong64(byte(str[n*8+1])) << 8 |
		   ulong64(byte(str[n*8+2])) << 16 | ulong64(byte(str[n*8+3])) << 24 |
		   ulong64(byte(str[n*8+4])) << 32 | ulong64(byte(str[n*8+5])) << 40 |
		   ulong64(byte(str[n*8+6])) << 48 | ulong64(byte(str[n*8+7])) << 56;
}

constexpr inline ulong64 rotl64c(ulong64 x, sbyte r)
{return (x << r) | (x >> (64 - r));}

constexpr inline ulong64 _downshift_and_xor(ulong64 k)
{return k ^ (k >> 33);}

constexpr inline ulong64 _calcblock_h(hash128 value, ulong64 h1, ulong64 h2)
{return (h2 + rotl64c(h1 ^ (_c2*rotl64c(value.h1*_c1, 31)), 27))*5 + 0x52dce729;}

constexpr inline hash128 _calcblock(hash128 value, ulong64 h1, ulong64 h2)
{
	return hash128(_calcblock_h(value, h1, h2),
		(_calcblock_h(value, h1, h2) + rotl64c(h2 ^ (_c1*rotl64c(value.h2*_c2, 33)), 31))*5 + 0x38495ab5);
}

constexpr inline hash128 _calcblocks(const char* str, size_t nblocks, size_t index, hash128 accum)
{
	return nblocks == 0? accum:
		index == nblocks-1?
		_calcblock(hash128(getU64(str, index*2+0), getU64(str, index*2+1)), accum.h1, accum.h2):
		_calcblocks(str, nblocks, index+1, _calcblock(hash128(getU64(str, index*2+0), getU64(str, index*2+1)), accum.h1, accum.h2));
}

constexpr inline hash128 _add(hash128 value)
{return hash128(value.h1+value.h2, value.h2*2+value.h1);}

constexpr inline ulong64 _fmix_64(ulong64 k)
{
	return _downshift_and_xor(_downshift_and_xor(
		_downshift_and_xor(k)*0xff51afd7ed558ccdULL)*0xc4ceb9fe1a85ec53ULL);
}

constexpr inline hash128 _fmix(hash128 value)
{return hash128(_fmix_64(value.h1), _fmix_64(value.h2));}

constexpr inline ulong64 _calcrest_xor(const char* str, size_t offset, size_t index, ulong64 k)
{return k ^ (ulong64(str[offset + index]) << (index * 8));}

constexpr inline ulong64 _calcrest_k(const char* str, size_t offset, size_t index, size_t len, ulong64 k)
{
	return index == (len-1)? _calcrest_xor(str, offset, index, k):
		_calcrest_xor(str, offset, index, _calcrest_k(str, offset, index+1, len, k));
}

constexpr inline hash128 _calcrest(const char* str, size_t offset, size_t restlen, hash128 value)
{
	return restlen==0? value:
		restlen>8? hash128(
			value.h1 ^ (rotl64c(_calcrest_k(str, offset, 0, restlen>8? 8: restlen, 0)*_c1, 31)*_c2),
			value.h2 ^ (rotl64c(_calcrest_k(str, offset+8, 0, restlen-8, 0)*_c2, 33)*_c1)):
		hash128(value.h1 ^ (rotl64c(_calcrest_k(str, offset, 0, restlen>8? 8: restlen, 0)*_c1, 31)*_c2),
			value.h2);
}

constexpr inline hash128 _calcfinal(size_t len, hash128 value)
{return _add(_fmix(_add(hash128(value.h1^len, value.h2^len))));}

}

constexpr hash128 inline Murmur3_128_x64(const char* str, size_t length, ulong64 seed)
{
	using namespace D;
	return _calcfinal(length,
		_calcrest(str, length/16*16, length & 15,
			_calcblocks(str, length/16, 0, hash128(seed, seed))
		)
	);
}

template<uint N> constexpr inline hash128 Murmur3_128_x64(const char(&key)[N], ulong64 seed)
{return Murmur3_128_x64(key, N-1, seed);}

constexpr inline ulong64 Murmur3_128_x64_low(const char* str, size_t length, ulong64 seed)
{return Murmur3_128_x64(str, length, seed).h1;}

template<uint N> constexpr inline ulong64 Murmur3_128_x64_low(const char(&key)[N], ulong64 seed)
{return Murmur3_128_x64(key, seed).h1;}

template<uint N> constexpr inline uint Murmur3_32(const char(&key)[N], uint seed)
{return D::murmur3_32_value(key, N-1, seed);}

#ifdef INTRA_CONSTEXPR_SUPPORT
static_assert(Murmur3_32("hello, world", 0) == 345750399, "murmur3 test 1");
static_assert(Murmur3_32("hello, world1", 0) == 3714214180, "murmur3 test 2");
static_assert(Murmur3_32("hello, world12", 0) == 83041023, "murmur3 test 3");
static_assert(Murmur3_32("hello, world123", 0) == 209220029, "murmur3 test 4");
static_assert(Murmur3_32("hello, world1234", 0) == 4241062699, "murmur3 test 5");
static_assert(Murmur3_32("hello, world", 1) == 1868346089, "murmur3 test 6");

static_assert(Murmur3_128_x64_low("hello, world", 0) == 0x342fac623a5ebc8eULL, "murmur3 128 test 1");
static_assert(Murmur3_128_x64_low("hello, world", 1) == 0x8b95f808840725c6ULL, "murmur3 128 test 2");
#endif

namespace Literals {

#ifdef INTRA_CONSTEXPR_SUPPORT

//! Murmur3 32-разрядная хеш-функция
constexpr inline uint operator"" _m3h(const char* str, size_t len)
{return D::murmur3_32_value(str, len, 0);}

//! Младшие 64 бита 128-разрядной Murmur3 хеш-функции
constexpr inline ulong64 operator"" _m3h64(const char* str, size_t len)
{return Murmur3_128_x64_low(str, len, 0);}

//! Murmur3 128-разрядная хеш-функция
constexpr inline hash128 operator"" _m3h128(const char* str, size_t len)
{return Murmur3_128_x64(str, len, 0);}

#endif

//! Используйте using namespace Intra::Algo::HashCT::Literals::Murmur3,
//! чтобы назначить эту версию хеша основной для строковых литералов.
namespace Murmur3 {

#ifdef INTRA_CONSTEXPR_SUPPORT

constexpr inline uint operator"" _h(const char* str, size_t len)
{return D::murmur3_32_value(str, len, 0);}

constexpr inline ulong64 operator"" _h64(const char* str, size_t len)
{return Murmur3_128_x64_low(str, len, 0);}

constexpr inline hash128 operator"" _h128(const char* str, size_t len)
{return Murmur3_128_x64(str, len, 0);}

#endif

}

}

}}}

#ifdef _MSC_VER
#pragma warning(pop)
#endif
