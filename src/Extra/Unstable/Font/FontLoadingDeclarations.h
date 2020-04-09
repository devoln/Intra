#pragma once

#include "Intra/Core.h"

//! Используемая библиотека для загрузки TBool<true> шрифтов
#define INTRA_LIBRARY_FONT_LOADING_Dummy 0
#define INTRA_LIBRARY_FONT_LOADING_STB 1
#define INTRA_LIBRARY_FONT_LOADING_FreeType 2
#define INTRA_LIBRARY_FONT_LOADING_Gdiplus 3
#define INTRA_LIBRARY_FONT_LOADING_Qt 4
#define INTRA_LIBRARY_FONT_LOADING_SDL 5

#ifndef INTRA_LIBRARY_FONT_LOADING

#ifdef _WIN32
#define INTRA_LIBRARY_FONT_LOADING INTRA_LIBRARY_FONT_LOADING_Dummy
//#define INTRA_LIBRARY_FONT_LOADING INTRA_LIBRARY_FONT_LOADING_Gdiplus
#else
#define INTRA_LIBRARY_FONT_LOADING INTRA_LIBRARY_FONT_LOADING_Dummy
#endif

#endif

INTRA_BEGIN
namespace FontLoadingAPI {

struct Font;
typedef Font* FontHandle;

}
INTRA_END
