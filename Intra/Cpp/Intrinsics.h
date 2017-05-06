#pragma once

#include "Fundamental.h"
#include "Compatibility.h"

#if !defined(_MSC_VER) || !defined(INTRA_AVOID_STD_HEADERS)
#include <string.h>
#include <stdlib.h>
#include <wchar.h>
#endif

namespace Intra {

#ifdef _MSC_VER
#define INTRA_DEBUG_BREAK __debugbreak()
#elif defined(__GNUC__)
#define INTRA_DEBUG_BREAK __builtin_trap()
#else
#include <signal.h>
#define INTRA_DEBUG_BREAK raise(SIGTRAP)
#endif

namespace C { extern "C" {

#if defined(_MSC_VER) && defined(INTRA_AVOID_STD_HEADERS)
const void* INTRA_CRTDECL memchr(const void* buf, int val, size_t maxCount);
int INTRA_CRTDECL memcmp(const void* buf1, const void* buf2, size_t size);
void* INTRA_CRTDECL memcpy(void* dst, const void* src, size_t size);
void* INTRA_CRTDECL memmove(void* _Dst, const void* src, size_t size);
void* INTRA_CRTDECL memset(void*, int val, size_t size);
size_t INTRA_CRTDECL strlen(const char* str) throw();
size_t INTRA_CRTDECL wcslen(const wchar_t* str) throw();

void* INTRA_CRTDECL malloc(size_t bytes) throw();
void* INTRA_CRTDECL realloc(void* oldPtr, size_t bytes) throw();
void INTRA_CRTDECL free(void* ptr) throw();
#else
using ::memchr;
using ::memcmp;
using ::memcpy;
using ::memmove;
using ::memset;
using ::strlen;
using ::wcslen;
using ::malloc;
using ::realloc;
using ::free;
#endif

}}}
