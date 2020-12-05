#pragma once

#include "IntraX/Core.h"

/// Используемая библиотека\API системы для загрузки изображений
#define INTRA_LIBRARY_IMAGE_LOADING_None 0
#define INTRA_LIBRARY_IMAGE_LOADING_STB 1
#define INTRA_LIBRARY_IMAGE_LOADING_DevIL 2
#define INTRA_LIBRARY_IMAGE_LOADING_Gdiplus 3
#define INTRA_LIBRARY_IMAGE_LOADING_Qt 4
#define INTRA_LIBRARY_IMAGE_LOADING_SDL 5
#define INTRA_LIBRARY_IMAGE_LOADING_Android 6

//Пытаемся автоматически определить доступную системную библиотеку для загрузки изображений
#ifndef INTRA_LIBRARY_IMAGE_LOADING

#ifdef _WIN32
#define INTRA_LIBRARY_IMAGE_LOADING INTRA_LIBRARY_IMAGE_LOADING_Gdiplus
#elif defined(__ANDROID__)
//TODO: сделать загрузку изображений средствами Java через JNI
#define INTRA_LIBRARY_IMAGE_LOADING INTRA_LIBRARY_IMAGE_LOADING_None
#else
#define INTRA_LIBRARY_IMAGE_LOADING INTRA_LIBRARY_IMAGE_LOADING_None
#endif

#endif

#if(INTRA_LIBRARY_IMAGE_LOADING != INTRA_LIBRARY_IMAGE_LOADING_None)

#include "IntraX/Unstable/Image/AnyImage.h"

INTRA_BEGIN
/// Загрузить изображение средствами библиотек операционной системы
/// или сторонних библиотек, поддерживающих сразу множество форматов:
/// GDI+ (Windows)
/// Java API через JNI (Android)
/// Qt (стороння библиотека, входит во многие дистрибутивы Linux)
/// DevIL (сторонняя библиотека)
/// STB image (сторонняя библиотека)
/// SDL image (сторонняя библиотека)
AnyImage LoadWithPlatform(IInputStream& stream);
INTRA_END

#endif
