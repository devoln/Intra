#pragma once

#ifdef _MSC_VER

#if(!defined(__GNUC__) && !defined(__clang__))
#pragma execution_character_set("utf-8")
#endif

#ifdef INTRA_MINIMIZE_CRT
#define _NO_CRT_STDIO_INLINE
#endif

//#define _USE_32BIT_TIME_T
#define fseeko64(file, offset, origin) fseek(file, (long)(offset), (origin))
#define ftello64 _ftelli64 //_ftelli64 не работает на старой CRT

#define _ATL_SECURE_NO_DEPRECATE
#define _CRT_SECURE_NO_WARNINGS
#define _CRT_NONSTDC_NO_WARNINGS

#elif(defined(__clang__))

extern "C" char* gets(char* str); //Затыкаем ошибку в стандартной библиотеке glibc, из-за которой clang не компилирует

#endif

#ifdef _MSC_VER
#define INTRA_CRTDECL __cdecl
#else
#define INTRA_CRTDECL
#endif

#ifdef _MSC_VER
//Clang 3.7 with Microsoft CodeGen (v140_clang_3_7) почему-то без этого не компилирует
#define _ALLOW_KEYWORD_MACROS
#endif
