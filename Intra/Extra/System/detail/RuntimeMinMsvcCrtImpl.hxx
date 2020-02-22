#include "Core/Core.h"
#include "Core/Numeric.h"

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include "Windows.h"

#define INTRA_NOT_LINK_CRT_LIB

extern "C"
{
	int _fltused;

#ifdef _M_IX86 // following functions are needed only for 32-bit architecture

	// Float to long
	void __declspec(naked) _ftol()
	{
		/* *INDENT-OFF* */
		__asm {
			push        ebp
			mov         ebp,esp
			sub         esp,20h
			and         esp,0FFFFFFF0h
			fld         st(0)
			fst         dword ptr [esp+18h]
			fistp       qword ptr [esp+10h]
			fild        qword ptr [esp+10h]
			mov         edx,dword ptr [esp+18h]
			mov         eax,dword ptr [esp+10h]
			test        eax,eax
			je          integer_QnaN_or_zero
	arg_is_not_integer_QnaN:
			fsubp       st(1),st
			test        edx,edx
			jns         positive
			fstp        dword ptr [esp]
			mov         ecx,dword ptr [esp]
			xor         ecx,80000000h
			add         ecx,7FFFFFFFh
			adc         eax,0
			mov         edx,dword ptr [esp+14h]
			adc         edx,0
			jmp         localexit
	positive:
			fstp        dword ptr [esp]
			mov         ecx,dword ptr [esp]
			add         ecx,7FFFFFFFh
			sbb         eax,0
			mov         edx,dword ptr [esp+14h]
			sbb         edx,0
			jmp         localexit
	integer_QnaN_or_zero:
			mov         edx,dword ptr [esp+14h]
			test        edx,7FFFFFFFh
			jne         arg_is_not_integer_QnaN
			fstp        dword ptr [esp+18h]
			fstp        dword ptr [esp+18h]
	localexit:
			leave
			ret
		}
		/* *INDENT-ON* */
	}

	// These functions are not exactly as regular casts. They do rounding differently (round to nearest, not truncate).
	// Also they don't process correctly NaNs and don't produce floating point exceptions.
	__declspec(naked) void _ftol2()
	{
		__asm
		{
			fistp qword ptr[esp-8]
			mov   edx, [esp-4]
			mov   eax, [esp-8]
			ret
		}
	}

#if INTRA_DISABLE
	__declspec(naked) void _ftol2_sse()
	{
		__asm
		{
			fistp dword ptr[esp-4]
			mov   eax, [esp-4]
			ret
		}
	}
#endif

	void _ftol2_sse()
	{
		_ftol();
	}

	// these functions are needed for SSE code for 32-bit arch,
	// TODO: they are broken, implement them correctly
	long long _dtol3(double x)
	{
		using namespace Intra;
		double x2 = x < 0? -x: x;
		if(x2 <= LMaxOf(int())) return int(x);
		uint64 res = unsigned(x2);
		x2 /= double(1ull << 32);
		res |= uint64(unsigned(x2)) << 32;
		return int64(res)*(x < 0? -1: 1);
	}


	unsigned long long _dtoul3(double x)
	{
		using namespace Intra;
		if(x <= LMaxOf(uint())) return uint(x);
		return unsigned(x) | (uint64(unsigned(x / double(1ull << 32))) << 32);
	}

	unsigned _dtoui3(double x)
	{
		if(x > 2147483648.0) return unsigned(int(x - 2147483648.0) ^ 0x80000000);
		return unsigned(int(x));
	}

	long long _ftol3(float x) {return _dtol3(x);}

	unsigned long long _ftoul3(float x) {return _dtoul3(x);}

	unsigned _ftoui3(float x)
	{
		if(x > 2147483648.0f) return unsigned(int(x - 2147483648.0f) ^ 0x80000000);
		return unsigned(int(x));
	}


	double _ltod3(long long x)
	{
		return double(int(x >> 32))*(1ll << 32)+int(x < 0? x & 0xFFFFFFFF:);
	}


	double _ultod3(unsigned long long x)
	{
		return 0;
	}
#endif
}

#ifdef _M_IX86
// Implementation for 64-bit operations on 32 bit x86 platform
#include "System/detail/RuntimeMinMsvcCrtInt64Impl.hxx"
#endif

#if INTRA_DISABLED
extern "C" int INTRA_CRTDECL _purecall() { return 0; }

#include <intrin.h>

extern "C" {

#pragma function(memset)
	void* memset(void* dst, int c, size_t count)
	{
		char* bytes = (char*)dst;
		while(count --> 0) *bytes++ = (char)c;
		return dst;
	}

#pragma function(memcpy)
	void* memcpy(void* destination, const void* source, size_t num)
	{
		auto src = static_cast<const unsigned char*>(source);
		auto dst = static_cast<unsigned char*>(destination);

		if(((uintptr_t)src & 15) == 0 && ((uintptr_t)dst & 15) == 0)
		{
			for(size_t i = num / 64; i --> 0;)
			{
				_mm_prefetch(reinterpret_cast<const char*>(src), _MM_HINT_NTA);
				__m128 values[4];
				values[0] = *reinterpret_cast<const __m128*>(src + 0);
				values[1] = *reinterpret_cast<const __m128*>(src + 16);
				values[2] = *reinterpret_cast<const __m128*>(src + 32);
				values[3] = *reinterpret_cast<const __m128*>(src + 48);
				_mm_stream_ps(reinterpret_cast<float*>(dst + 0), values[0]);
				_mm_stream_ps(reinterpret_cast<float*>(dst + 16), values[1]);
				_mm_stream_ps(reinterpret_cast<float*>(dst + 32), values[2]);
				_mm_stream_ps(reinterpret_cast<float*>(dst + 48), values[3]);
				src += 64;
				dst += 64;
			}
			num &= 63;
		}

		while(num --> 0) *dst++ = *src++;
		return destination;
	}
}
#endif

//#define _CRTALLOC(x) __declspec(allocate(x))
typedef void(INTRA_CRTDECL* _PVFV)();

#pragma data_seg(".CRT$XCA")    // C++ initializers
_PVFV __xc_a[] = {NULL};
#pragma data_seg(".CRT$XCZ")
_PVFV __xc_z[] = {NULL};

#pragma data_seg(".CRT$XIA")    // C initializers
_PVFV __xi_a[] = {NULL};
#pragma data_seg(".CRT$XIZ")
_PVFV __xi_z[] = {NULL};
#pragma data_seg(".CRT$XPA")    // C pre-terminators
_PVFV __xp_a[] = {NULL};
#pragma data_seg(".CRT$XPZ")
_PVFV __xp_z[] = {NULL};
#pragma data_seg(".CRT$XTA")    // C terminators
_PVFV __xt_a[] = {NULL};
#pragma data_seg(".CRT$XTZ")
_PVFV __xt_z[] = {NULL};
#pragma data_seg()
#pragma comment(linker, "/merge:.CRT=.data")

/*extern _CRTALLOC(".CRT$XIA") _PVFV __xi_a[];
extern _CRTALLOC(".CRT$XIZ") _PVFV __xi_z[];    // C initializers
extern _CRTALLOC(".CRT$XCA") _PVFV __xc_a[];
extern _CRTALLOC(".CRT$XCZ") _PVFV __xc_z[];    // C++ initializers

extern _CRTALLOC(".CRT$XPA") _PVFV __xp_a[];
extern _CRTALLOC(".CRT$XPZ") _PVFV __xp_z[];
extern _CRTALLOC(".CRT$XTA") _PVFV __xt_a[];
extern _CRTALLOC(".CRT$XTZ") _PVFV __xt_z[];*/


#ifndef CRTDLL
static
#endif
void INTRA_CRTDECL _initterm(_PVFV* pfbegin, _PVFV* pfend)
{
	while(pfbegin < pfend)
	{
		if(*pfbegin) (**pfbegin)();
		++pfbegin;
	}
}

HANDLE g_hHeap;

extern "C" int INTRA_CRTDECL main(int argc, char* argv[]);
extern "C" void mainCRTStartup()
{
	g_hHeap = HeapCreate(0, 8*1048576, 3072*1048576);
	_initterm(__xi_a, __xi_z);
	_initterm(__xc_a, __xc_z);

	int argc = 0;
	char* argv[] = {""};
	main(argc, argv);

	/* and finally... */
	_initterm(__xp_a, __xp_z);    /* Pre-termination (C++?) */
	_initterm(__xt_a, __xt_z);    /* Termination */
	HeapDestroy(g_hHeap);
}


#ifdef __cpp_exceptions
extern "C"
{
#if _M_IX86
	EXCEPTION_DISPOSITION _except_handler3(
			EXCEPTION_RECORD* ExceptionRecord,
			void* EstablisherFrame,
			struct _CONTEXT* ContextRecord,
			void* DispatcherContext)
	{
		typedef EXCEPTION_DISPOSITION Function(EXCEPTION_RECORD*, void*, struct _CONTEXT*, void*);
		static Function* FunctionPtr;

		if(!FunctionPtr)
		{
			HMODULE Library = LoadLibraryA("msvcrt.dll");
			FunctionPtr = (Function*)GetProcAddress(Library, "_except_handler3");
		}

		return FunctionPtr(ExceptionRecord, EstablisherFrame, ContextRecord, DispatcherContext);
	}

	UINT_PTR __security_cookie = 0xBB40E64E;

	extern PVOID __safe_se_handler_table[];
	extern BYTE  __safe_se_handler_count;

	struct IMAGE_LOAD_CONFIG_DIRECTORY32_2
	{
		DWORD Size, TimeDateStamp;
		WORD MajorVersion, MinorVersion;
		DWORD GlobalFlagsClear, GlobalFlagsSet, CriticalSectionDefaultTimeout,
			DeCommitFreeBlockThreshold, DeCommitTotalFreeThreshold, LockPrefixTable, MaximumAllocationSize,
			VirtualMemoryThreshold, ProcessHeapFlags, ProcessAffinityMask;
		WORD CSDVersion, Reserved1;
		DWORD EditList;
		PUINT_PTR SecurityCookie;
		void** SEHandlerTable;
		DWORD SEHandlerCount;
	};

	const IMAGE_LOAD_CONFIG_DIRECTORY32_2 _load_config_used = {
		sizeof(IMAGE_LOAD_CONFIG_DIRECTORY32_2),
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		&__security_cookie,
		__safe_se_handler_table,
		DWORD(DWORD_PTR(&__safe_se_handler_count))
	};
#elif _M_AMD64
	EXCEPTION_DISPOSITION __C_specific_handler(
			struct _EXCEPTION_RECORD* ExceptionRecord,
			void* EstablisherFrame,
			struct _CONTEXT* ContextRecord,
			struct _DISPATCHER_CONTEXT* DispatcherContext)
	{
		typedef EXCEPTION_DISPOSITION Function(EXCEPTION_RECORD*, void*, struct _CONTEXT*, _DISPATCHER_CONTEXT*);
		static Function* FunctionPtr;
		if(!FunctionPtr)
		{
			HMODULE Library = LoadLibraryA("msvcrt.dll");
			FunctionPtr = (Function*)GetProcAddress(Library, "__C_specific_handler");
		}
		return FunctionPtr(ExceptionRecord, EstablisherFrame, ContextRecord, DispatcherContext);
	}
#endif
}
#endif
