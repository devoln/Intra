#pragma once

#ifndef INTRA_MINEXE
#define INTRA_MINEXE 0
#endif

//! Типы процессоров
#define INTRA_PLATFORM_X86 0
#define INTRA_PLATFORM_X86_64 1
#define INTRA_PLATFORM_ARM 2
#define INTRA_PLATFORM_PowerPC 3
#define INTRA_PLATFORM_MIPS 4
#define INTRA_PLATFORM_IA64 5
#define INTRA_PLATFORM_ARM64 6
#define INTRA_PLATFORM_Emscripten 7

//! Порядок байтов
#define INTRA_PLATFORM_ENDIANESS_LittleEndian 0
#define INTRA_PLATFORM_ENDIANESS_BigEndian 1
#define INTRA_PLATFORM_ENDIANESS_MixedEndian 2

//! Типы операционной системы
#define INTRA_PLATFORM_OS_Windows 0
#define INTRA_PLATFORM_OS_WindowsPhone 1
#define INTRA_PLATFORM_OS_Linux 2
#define INTRA_PLATFORM_OS_Android 3
#define INTRA_PLATFORM_OS_FreeBSD 4
#define INTRA_PLATFORM_OS_iOS 5
#define INTRA_PLATFORM_OS_MacOS 6
#define INTRA_PLATFORM_OS_Emscripten 7

//! Типы оконной системы
#define INTRA_LIBRARY_WINDOW_SYSTEM_Console 0
#define INTRA_LIBRARY_WINDOW_SYSTEM_Windows 1
#define INTRA_LIBRARY_WINDOW_SYSTEM_X11 2
#define INTRA_LIBRARY_WINDOW_SYSTEM_Qt 3
#define INTRA_LIBRARY_WINDOW_SYSTEM_SDL 4
#define INTRA_LIBRARY_WINDOW_SYSTEM_AndroidNativeActivity 5
#define INTRA_LIBRARY_WINDOW_SYSTEM_WinRT 6
#define INTRA_LIBRARY_WINDOW_SYSTEM_Emscripten 7

//! Поддержка многопоточности
#define INTRA_LIBRARY_THREADING_Dummy 0
#define INTRA_LIBRARY_THREADING_WinAPI 1
#define INTRA_LIBRARY_THREADING_CPPLIB 2
#define INTRA_LIBRARY_THREADING_Qt 3
#define INTRA_LIBRARY_THREADING_SDL 4
#define INTRA_LIBRARY_THREADING_PThread 5

//! Используемая библиотека для вывода графики
#define INTRA_LIBRARY_GRAPHICS_OpenGL 0
#define INTRA_LIBRARY_GRAPHICS_OpenGLES 1
#define INTRA_LIBRARY_GRAPHICS_Vulkan 2
#define INTRA_LIBRARY_GRAPHICS_Direct3D11 3
#define INTRA_LIBRARY_GRAPHICS_Direct3D12 4

//! Используемый метод для отсчёта времени
#define INTRA_LIBRARY_TIMER_Dummy 0
#define INTRA_LIBRARY_TIMER_QPC 1
#define INTRA_LIBRARY_TIMER_CPPLIB 2
#define INTRA_LIBRARY_TIMER_Qt 3
#define INTRA_LIBRARY_TIMER_CLIB 4


//Пытаемся автоматически определить ОС и используемую графическую подсистему
#if defined(WIN32) || defined(_WIN32) || defined(WIN64) || defined(_WIN64)

#ifndef INTRA_PLATFORM_OS
#define INTRA_PLATFORM_OS INTRA_PLATFORM_OS_Windows
#endif

#endif

#ifdef __EMSCRIPTEN__
#define INTRA_PLATFORM_OS INTRA_PLATFORM_OS_Emscripten

#ifndef INTRA_PLATFORM_ARCH
#define INTRA_PLATFORM_ARCH INTRA_PLATFORM_Emscripten
#endif

#ifndef INTRA_LIBRARY_WINDOW_SYSTEM
#define INTRA_LIBRARY_WINDOW_SYSTEM INTRA_LIBRARY_WINDOW_SYSTEM_Emscripten
#endif

#endif

#ifdef __ANDROID__

#ifndef INTRA_PLATFORM_OS
#define INTRA_PLATFORM_OS INTRA_PLATFORM_OS_Android
#endif

#ifndef INTRA_LIBRARY_WINDOW_SYSTEM
#define INTRA_LIBRARY_WINDOW_SYSTEM INTRA_LIBRARY_WINDOW_SYSTEM_AndroidNativeActivity
#endif

#endif

#ifdef __linux__

#ifndef INTRA_PLATFORM_OS
#define INTRA_PLATFORM_OS INTRA_PLATFORM_OS_Linux
#endif

#endif

#ifdef __FreeBSD__

#ifndef INTRA_PLATFORM_OS
#define INTRA_PLATFORM_OS INTRA_PLATFORM_OS_FreeBSD
#endif

#endif

#ifndef INTRA_PLATFORM_ARCH

#if defined(WIN64) || defined(_WIN64)
#define INTRA_PLATFORM_ARCH INTRA_PLATFORM_X86_64
#elif(defined(WIN32) || defined(_WIN32))
#define INTRA_PLATFORM_ARCH INTRA_PLATFORM_X86
#endif

#endif

#ifndef INTRA_PLATFORM_ENDIANESS
#define INTRA_PLATFORM_ENDIANESS INTRA_PLATFORM_ENDIANESS_LittleEndian
#endif

#ifndef INTRA_LIBRARY_WINDOW_SYSTEM

#if(INTRA_PLATFORM_OS==INTRA_PLATFORM_OS_Windows)
#define INTRA_LIBRARY_WINDOW_SYSTEM INTRA_LIBRARY_WINDOW_SYSTEM_Windows
#else
#define INTRA_LIBRARY_WINDOW_SYSTEM INTRA_LIBRARY_WINDOW_SYSTEM_X11
#endif

#endif


//Пытаемся автоматически определить аппаратную архитектуру
#ifndef INTRA_PLATFORM_ARCH

#if defined(__i386__) || defined(__i686__) || defined(_M_IX86)
#define INTRA_PLATFORM_ARCH INTRA_PLATFORM_X86
#endif

#if defined(__amd64__) || defined(_M_AMD64) || defined(_M_X64)
#define INTRA_PLATFORM_ARCH INTRA_PLATFORM_X86_64
#endif


#if(defined(__aarch64__) || defined(_M_ARM64) || defined(__arm64))
#define INTRA_PLATFORM_ARCH INTRA_PLATFORM_ARM64
#elif defined(__arm__) || defined(__thumb__) || defined(_M_ARM) || defined(_M_ARMT)

#ifdef __LP64__
#define INTRA_PLATFORM_ARCH INTRA_PLATFORM_ARM64
#else
#define INTRA_PLATFORM_ARCH INTRA_PLATFORM_ARM
#endif

#endif

#if defined(__powerpc__) || defined(_M_PPC) || defined(__powerpc)
#define INTRA_PLATFORM_ARCH INTRA_PLATFORM_PowerPC
#endif

#if defined(__mips__) || defined(__MIPS__) || defined(__mips)
#define INTRA_PLATFORM_ARCH INTRA_PLATFORM_MIPS
#endif

#if defined(__ia64__) || defined(_M_IA64) || defined(__IA64__)
#define INTRA_PLATFORM_ARCH INTRA_PLATFORM_IA64
#endif

#endif

#if(INTRA_PLATFORM_ARCH==INTRA_PLATFORM_X86_64 ||\
	INTRA_PLATFORM_ARCH==INTRA_PLATFORM_IA64 ||\
	INTRA_PLATFORM_ARCH==INTRA_PLATFORM_ARM64)
#define INTRA_PLATFORM_IS_64
#endif

//Пытаемся автоматически определить порядок байтов
#if !defined(INTRA_PLATFORM_ENDIANESS) && defined(__BYTE_ORDER__)

#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
#define INTRA_PLATFORM_ENDIANESS INTRA_PLATFORM_ENDIANESS_LittleEndian
#elif __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
#define INTRA_PLATFORM_ENDIANESS INTRA_PLATFORM_ENDIANESS_BigEndian
#elif __BYTE_ORDER__ == __ORDER_PDP_ENDIAN__
#define INTRA_PLATFORM_ENDIANESS INTRA_PLATFORM_ENDIANESS_MixedEndian
#endif

#endif

#ifndef INTRA_PLATFORM_ENDIANESS

#if(INTRA_PLATFORM_ARCH==INTRA_PLATFORM_X86 || INTRA_PLATFORM_ARCH==INTRA_PLATFORM_X86_64)
#define INTRA_PLATFORM_ENDIANESS INTRA_PLATFORM_ENDIANESS_LittleEndian
#endif

#endif



#if(INTRA_PLATFORM_OS==INTRA_PLATFORM_OS_Linux || INTRA_PLATFORM_OS==INTRA_PLATFORM_OS_FreeBSD ||\
	defined(unix) || defined(__unix) || defined(__unix__))
#define INTRA_PLATFORM_IS_UNIX
#endif

#if defined(INTRA_PLATFORM_IS_UNIX) && !defined(INTRA_LIBRARY_WINDOW_SYSTEM)
#define INTRA_LIBRARY_WINDOW_SYSTEM INTRA_LIBRARY_WINDOW_SYSTEM_X11 //Может быть несколько вариантов, но это вариант по умолчанию
#endif

#if(INTRA_PLATFORM_OS==INTRA_PLATFORM_OS_Android ||\
	INTRA_PLATFORM_OS==INTRA_PLATFORM_OS_iOS ||\
	INTRA_PLATFORM_OS==INTRA_PLATFORM_OS_Emscripten)
#define INTRA_LIBRARY_GRAPHICS INTRA_LIBRARY_GRAPHICS_OpenGLES
#else
#define INTRA_LIBRARY_GRAPHICS INTRA_LIBRARY_GRAPHICS_OpenGL
#endif


//Пытаемся автоматически определить библиотеку для работы с потоками
#ifndef INTRA_LIBRARY_THREADING

#if(INTRA_PLATFORM_OS==INTRA_PLATFORM_OS_Windows)
#define INTRA_LIBRARY_THREADING INTRA_LIBRARY_THREADING_WinAPI
#elif(INTRA_PLATFORM_OS==INTRA_PLATFORM_OS_Emscripten)

#ifdef __EMSCRIPTEN_PTHREADS__ 
#define INTRA_LIBRARY_THREADING INTRA_LIBRARY_THREADING_PThread
#else
#define INTRA_LIBRARY_THREADING INTRA_LIBRARY_THREADING_Dummy
#endif

#else
#define INTRA_LIBRARY_THREADING INTRA_LIBRARY_THREADING_PThread
#endif

#endif

#ifndef INTRA_LIBRARY_TIMER

#if INTRA_PLATFORM_OS==INTRA_PLATFORM_OS_Windows
#define INTRA_LIBRARY_TIMER INTRA_LIBRARY_TIMER_QPC
#else
#define INTRA_LIBRARY_TIMER INTRA_LIBRARY_TIMER_CPPLIB
#endif

#endif


//Если что-то не было задано и не удалось определить, сообщаем об ошибке
#ifndef INTRA_PLATFORM_ARCH
#error Невозможно определить архитектуру целевого процессора!
#endif

#ifndef INTRA_PLATFORM_OS
#error Невозможно определить целевую ОС!
#endif

#ifndef INTRA_LIBRARY_WINDOW_SYSTEM
#error Невозможно определить API для работы с окнами!
#endif

#ifndef INTRA_PLATFORM_ENDIANESS
#error Не удаётся определить порядок байтов для данной платформы!
#endif



#ifndef INTRA_NO_FILE_LOGGING

#if(INTRA_PLATFORM_OS==INTRA_PLATFORM_OS_Emscripten)
#define INTRA_NO_FILE_LOGGING
#endif

#endif
