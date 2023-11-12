#pragma once

#include <Intra/Core.h>

namespace Intra { INTRA_BEGIN

// For copying small blocks up to 4 bytes. Bigger objects will fallback to slow memcpy
inline void MemoryCopyTiny(char* dst, const char* src, size_t n)
{
    if(n <= 4)
    {
        if(n >= 2)
        {
            *(short*)dst = *(short*)src;
            *(short*)(dst + n - 2) = *(short*)(src + n - 2);
        }
        else *dst = *src;
    }
    else __builtin_memcpy(dst, src, n);
}

} INTRA_END
