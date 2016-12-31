#pragma once

#ifdef _MSC_VER

#ifndef __PLACEMENT_NEW_INLINE
#define __PLACEMENT_NEW_INLINE
inline void* operator new(size_t, void* dst) {return dst;}
inline void operator delete(void*, void*) {return;}
#endif

#ifndef __PLACEMENT_VEC_NEW_INLINE
#define __PLACEMENT_VEC_NEW_INLINE
inline void* operator new[](size_t, void* dst) {return dst;}
inline void operator delete[](void*, void*) {}
#endif

#else

#include <new>

#endif
