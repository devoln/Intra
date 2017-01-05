#pragma once

#ifdef _MSC_VER

INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS

#ifndef __PLACEMENT_NEW_INLINE
#define __PLACEMENT_NEW_INLINE
inline void* operator new(size_t, void* dst) {return dst;}
inline void operator delete(void*, void*) {}
#endif

#ifndef __PLACEMENT_VEC_NEW_INLINE
#define __PLACEMENT_VEC_NEW_INLINE
inline void* operator new[](size_t, void* dst) {return dst;}
inline void operator delete[](void*, void*) {}
#endif

INTRA_WARNING_POP

#else

#include <new>

#endif
