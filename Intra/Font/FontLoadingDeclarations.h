#pragma once

#include "Cpp/PlatformDetect.h"

//! Используемая библиотека для загрузки TrueType шрифтов
#define INTRA_LIBRARY_FONT_LOADING_Dummy 0
#define INTRA_LIBRARY_FONT_LOADING_STB 1
#define INTRA_LIBRARY_FONT_LOADING_FreeType 2
#define INTRA_LIBRARY_FONT_LOADING_Gdiplus 3
#define INTRA_LIBRARY_FONT_LOADING_Qt 4
#define INTRA_LIBRARY_FONT_LOADING_SDL 5

#ifndef INTRA_LIBRARY_FONT_LOADING

#if(INTRA_PLATFORM_OS==INTRA_PLATFORM_OS_Windows)
#define INTRA_LIBRARY_FONT_LOADING INTRA_LIBRARY_FONT_LOADING_Dummy
//#define INTRA_LIBRARY_FONT_LOADING INTRA_LIBRARY_FONT_LOADING_Gdiplus
#else
#define INTRA_LIBRARY_FONT_LOADING INTRA_LIBRARY_FONT_LOADING_Dummy
#endif

#endif

namespace Intra { namespace FontLoadingAPI {

struct Font;
typedef Font* FontHandle;

}}
