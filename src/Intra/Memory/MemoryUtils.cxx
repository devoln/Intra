#include "MemoryUtils.h"

namespace Intra { INTRA_BEGIN

struct XMM {long long ll[2];};
// For copying small blocks up to 4 bytes. Bigger objects will fallback to slow memcpy.
// On 4-byte blocks 1.5 times faster than memcpy.
INTRA_NOINLINE void MemocyCopy32(char* dst, const char* src, size_t n)
{
    switch(__builtin_clz(n))
    {
    default: __builtin_memcpy(dst, src, n); return;
    case 27:
        *(XMM*)dst = *(XMM*)src;
        *(XMM*)(dst + (n & 15)) = *(XMM*)(src + (n & 15));
        break;
    case 28:
        *(long long*)dst = *(long long*)src;
        *(long long*)(dst + (n & 7)) = *(long long*)(src + (n & 7));
        break;
    case 29:
        *(int*)dst = *(int*)src;
        *(int*)(dst + (n & 3)) = *(int*)(src + (n & 3));
        break;
    case 30:
        *(short*)dst = *(short*)src;
        *(short*)(dst + (n & 1)) = *(short*)(src + (n & 1));
        break;
    case 31:
        *dst = *src;
        break;
    };
}

} INTRA_END
