#pragma once

#ifdef _MSC_VER

#if(!defined(__GNUC__) && !defined(__clang__))
#pragma execution_character_set("utf-8")
#endif

#elif(defined(__clang__))

//extern "C" char* gets(char* str); //Затыкаем ошибку в стандартной библиотеке glibc, из-за которой clang не компилирует

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
