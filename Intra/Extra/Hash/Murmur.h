#pragma once

#include "Types.h"

#include "Core/Range/StringView.h"

INTRA_BEGIN
namespace Hash {

uint Murmur3_32(StringView key, uint seed);
hash128 Murmur3_128_x64(StringView key, uint seed);
hash128 Murmur3_128_x32(StringView key, uint seed);
uint64 Murmur2_64_x64(StringView key, uint seed);
uint64 Murmur2_64_x32(StringView key, uint seed);

inline uint Murmur3_32(const char* key, uint seed)
{return Murmur3_32(StringView(key), seed);}

inline hash128 Murmur3_128_x64(const char* key, uint seed)
{return Murmur3_128_x64(StringView(key), seed);}

inline hash128 Murmur3_128_x32(const char* key, uint seed)
{return Murmur3_128_x32(StringView(key), seed);}

inline uint64 Murmur2_64_x64(const char* key, uint seed)
{return Murmur2_64_x64(StringView(key), seed);}

inline uint64 Murmur2_64_x32(const char* key, uint seed)
{return Murmur2_64_x32(StringView(key), seed);}

}
INTRA_END
