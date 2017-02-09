#pragma once

#include "Platform/PlatformInfo.h"

//! Используемая библиотека\API системы для загрузки изображений
#define INTRA_LIBRARY_IMAGE_LOADING_None 0
#define INTRA_LIBRARY_IMAGE_LOADING_STB 1
#define INTRA_LIBRARY_IMAGE_LOADING_DevIL 2
#define INTRA_LIBRARY_IMAGE_LOADING_Gdiplus 3
#define INTRA_LIBRARY_IMAGE_LOADING_Qt 4
#define INTRA_LIBRARY_IMAGE_LOADING_SDL 5
#define INTRA_LIBRARY_IMAGE_LOADING_Android 6

//Пытаемся автоматически определить доступную системную библиотеку для загрузки изображений
#ifndef INTRA_LIBRARY_IMAGE_LOADING

#if(INTRA_PLATFORM_OS==INTRA_PLATFORM_OS_Windows)
#define INTRA_LIBRARY_IMAGE_LOADING INTRA_LIBRARY_IMAGE_LOADING_Gdiplus
#elif(INTRA_PLATFORM_OS==INTRA_PLATFORM_OS_Android)
//TODO: сделать загрузку изображений через JNI
#define INTRA_LIBRARY_IMAGE_LOADING INTRA_LIBRARY_IMAGE_LOADING_None
#else
#define INTRA_LIBRARY_IMAGE_LOADING INTRA_LIBRARY_IMAGE_LOADING_None
#endif

#endif

#if(INTRA_LIBRARY_IMAGE_LOADING != INTRA_LIBRARY_IMAGE_LOADING_None)

#include "Image/AnyImage.h"

namespace Intra { namespace Image {

//! Загрузить изображение средствами библиотек операционной системы
//! или сторонних библиотек, поддерживающих сразу множество форматов:
//! GDI+ (Windows)
//! Java API через JNI (Android)
//! Qt (стороння библиотека, входит во многие дистрибутивы Linux)
//! DevIL (сторонняя библиотека)
//! STB image (сторонняя библиотека)
//! SDL image (сторонняя библиотека)
AnyImage LoadWithPlatform(IO::IInputStream& stream, size_t bytes);

}}

#endif
