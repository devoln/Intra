#include "Core/Core.h"

#define _CRT_RAND_S
#include <cstdlib>
namespace std {

unsigned int INTRA_CRTDECL _Random_device()
{
	unsigned int ans;
	if(rand_s(&ans)) abort();
	return (ans);
}

}

#if(_MSC_VER >= 1800)

#include <stdlib.h>

//extern "C" int _imp___fdtest(float*) {return -1;} //Это заглушка для недостающего символа. Она может вызвать ошибки с математической библиотекой

namespace std {
void _cdecl _Xbad_alloc() { abort(); }
void _cdecl _Xbad_function_call() { abort(); }
void _cdecl _Xout_of_range(const char*) { abort(); }
void _cdecl _Xlength_error(const char*) { abort(); }
const char* _cdecl _Syserror_map(int) { return "error"; }
const char* _cdecl _Winerror_map(int) { return "error"; }
}
extern "C" void _cdecl _except_handler4_common() {}

#if !defined(INTRA_INLINE_MATH) && defined(INTRA_MINIMIZE_CRT)
#include <math.h>
extern "C"
{
#ifndef _ACRTIMP
#define _ACRTIMP
#endif
	_Check_return_ _ACRTIMP double INTRA_CRTDECL round(_In_ double x);
	_Check_return_ _ACRTIMP float INTRA_CRTDECL roundf(_In_ float x);
	_Check_return_ _ACRTIMP double INTRA_CRTDECL round(_In_ double x) { return ::floor(x+0.5); }
	_Check_return_ _ACRTIMP float INTRA_CRTDECL roundf(_In_ float x) { return ::floorf(x+0.5f); }
}
#endif

#endif

#if(_MSC_VER >= 1900)

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <Windows.h>
#include <cstdio>

//thread_safe_statics.obj;utility_desktop.obj

void __CRTDECL operator delete(void* block, decltype(sizeof(0))) noexcept
{
	operator delete(block);
}

extern "C" {

	FILE* _cdecl __acrt_iob_func(unsigned int id)
	{
		static FILE* const stds[] = {_fdopen(0, "r"), _fdopen(1, "w"), _fdopen(2, "w")};
		return stds[id];
	}

	BOOL INTRA_CRTDECL __vcrt_InitializeCriticalSectionEx(LPCRITICAL_SECTION critical_section, DWORD spin_count, DWORD flags)
	{
		/*if(auto const initialize_critical_section_ex = try_get_InitializeCriticalSectionEx())
		{
			return initialize_critical_section_ex(critical_section, spin_count, flags);
		}*/
		(void)flags;
		return InitializeCriticalSectionAndSpinCount(critical_section, spin_count);
	}

	void INTRA_CRTDECL terminate() { abort(); }
	extern "C" void INTRA_CRTDECL __std_terminate() { abort(); }
	void INTRA_CRTDECL _invalid_parameter_noinfo_noreturn() { abort(); }
	void _fastcall _guard_check_icall(unsigned int) {}

}

#endif
