#pragma once

#include "Types.h"

#include "Intra/Range/StringView.h"

namespace Intra { INTRA_BEGIN
namespace Hash {

unsigned Murmur3_32(StringView key, unsigned seed);
hash128 Murmur3_128_x64(StringView key, unsigned seed);
hash128 Murmur3_128_x32(StringView key, unsigned seed);
uint64 Murmur2_64_x64(StringView key, unsigned seed);
uint64 Murmur2_64_x32(StringView key, unsigned seed);

inline unsigned Murmur3_32(const char* key, unsigned seed)
{return Murmur3_32(StringView(key), seed);}

inline hash128 Murmur3_128_x64(const char* key, unsigned seed)
{return Murmur3_128_x64(StringView(key), seed);}

inline hash128 Murmur3_128_x32(const char* key, unsigned seed)
{return Murmur3_128_x32(StringView(key), seed);}

inline uint64 Murmur2_64_x64(const char* key, unsigned seed)
{return Murmur2_64_x64(StringView(key), seed);}

inline uint64 Murmur2_64_x32(const char* key, unsigned seed)
{return Murmur2_64_x32(StringView(key), seed);}

}
} INTRA_END
