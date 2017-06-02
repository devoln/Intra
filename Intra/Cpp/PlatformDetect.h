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

//Пытаемся автоматически определить ОС
#if defined(WIN32) || defined(_WIN32) || defined(WIN64) || defined(_WIN64)

#ifndef INTRA_PLATFORM_OS
#define INTRA_PLATFORM_OS INTRA_PLATFORM_OS_Windows
#endif

#endif

#ifdef __EMSCRIPTEN__

#ifndef INTRA_PLATFORM_OS
#define INTRA_PLATFORM_OS INTRA_PLATFORM_OS_Emscripten
#endif

#ifndef INTRA_PLATFORM_ARCH
#define INTRA_PLATFORM_ARCH INTRA_PLATFORM_Emscripten
#endif

#endif

#ifdef __ANDROID__

#ifndef INTRA_PLATFORM_OS
#define INTRA_PLATFORM_OS INTRA_PLATFORM_OS_Android
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

#if(INTRA_PLATFORM_OS == INTRA_PLATFORM_OS_Linux || INTRA_PLATFORM_OS == INTRA_PLATFORM_OS_FreeBSD ||\
	defined(unix) || defined(__unix) || defined(__unix__))
#define INTRA_PLATFORM_IS_UNIX
#endif


//Если что-то не было задано и не удалось определить, сообщаем об ошибке
#ifndef INTRA_PLATFORM_ARCH
#error Cannot determine target processor architecture!
#endif

#ifndef INTRA_PLATFORM_OS
#error Cannot determine target OS!
#endif

#ifndef INTRA_PLATFORM_ENDIANESS
#error Cannot determine byte order for this platform!
#endif

#ifndef INTRA_NO_FILE_LOGGING

#if(INTRA_PLATFORM_OS == INTRA_PLATFORM_OS_Emscripten)
#define INTRA_NO_FILE_LOGGING
#endif

#endif

#if(!defined(INTRA_PLATFORM_IS_64) || INTRA_PLATFORM_OS == INTRA_PLATFORM_OS_Windows ||\
	INTRA_PLATFORM_OS == INTRA_PLATFORM_OS_Emscripten)
#define INTRA_SIZEOF_LONG 4
#else
#define INTRA_SIZEOF_LONG 8
#endif
