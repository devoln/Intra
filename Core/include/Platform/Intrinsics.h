#pragma once

#include "Platform/FundamentalTypes.h"
#include "Platform/Compatibility.h"

#ifndef _MSC_VER
#include <string.h>
#include <stdlib.h>
#include <wchar.h>
#endif

namespace Intra { namespace C { extern "C" {

#ifdef _MSC_VER
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
