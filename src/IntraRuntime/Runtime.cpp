#include "Intra/Core.h"
#include "Extra/System/Runtime.h"

INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS
INTRA_IGNORE_WARNING("pedantic")

#ifndef INTRA_NO_THREAD_LOCAL_FIX
// It was necessary on FreeBSD 10, haven't checked newer versions.
// If it causes multiple definition error for __cxa_thread_atexit, disable this using #define INTRA_NO_THREAD_LOCAL_FIX
#include "Extra/System/detail/RuntimeThreadLocalFix.hxx"
#endif

#ifndef INTRA_NO_UNIFIED_MAIN
// Wrapper that provides the ability to define main function on any platform (defines WinMain or android_main wrapper).
#include "Extra/System/detail/RuntimeUnifiedMain.hxx"
#endif


#if defined(_MSC_VER) && defined(INTRA_MINIMIZE_CRT)
#define INTRA_NO_FULL_CRT
#include "Extra/System/detail/RuntimeMsvcrtOldHelper.hxx"
#endif

#ifdef INTRA_NO_CRT

// It is an attempt to completely avoid linking C runtime.
// It is not finished and not tested, may cause linker or runtime errors.
// If you disable CRT on MSVC compile with flag -arch:IA32 on 32-bit x86 platform. SSE float<->integer casts are not implemented.
// However you can selectively enable SSE to compile files that use intrinsics without C++-casts
// On MSVC compile with -GR- (RTTI is not supported), -EHa- (C++ exceptions are not supported)

#ifndef INTRA_NO_FULL_CRT
#define INTRA_NO_FULL_CRT
#endif

#ifdef _MSC_VER
#include "Extra/System/detail/RuntimeMinMsvcCrtImpl.hxx"
#endif

#ifdef _WIN32
void* INTRA_CRTDECL malloc(size_t bytes)
{return HeapAlloc(g_hHeap, 0, bytes);}

void* INTRA_CRTDECL realloc(void* ptr, size_t bytes)
{return HeapReAlloc(g_hHeap, 0, ptr, bytes);}

void INTRA_CRTDECL free(void* ptr)
{HeapFree(g_hHeap, 0, ptr);}
#else

#endif

void* INTRA_CRTDECL operator new(size_t bytes) noexcept
{return malloc(bytes);}

void INTRA_CRTDECL operator delete(void* block) noexcept
{free(block);}

void INTRA_CRTDECL operator delete(void* block, size_t) noexcept
{operator delete(block);}

#endif

#if defined(__i386__) && defined(_MSC_VER) && (defined(INTRA_NO_FULL_CRT) || defined(INTRA_QIFIST))
// Run it on startup if /QIfist is enabled
inline int SetFloatingPointRoundingToTruncate()
{
	short control_word, control_word2;
	__asm
	{
		fstcw   control_word                // store fpu control word
		mov     dx, word ptr[control_word]
		or      dx, 0x0C00                  // rounding: truncate
		mov     control_word2, dx
		fldcw   control_word2               // load modfied control word
	}
	return 0;
}

static int GLOBAL = SetFloatingPointRoundingToTruncate();
#endif

INTRA_WARNING_POP
