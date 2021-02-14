﻿#include "IntraX/Unstable/Font/FontLoading.h"
#include "Intra/Range/StringView.h"
#include "IntraX/Container/Sequential/String.h"
#include "IntraX/Math/Vector2.h"


#if INTRA_LIBRARY_FONT_LOADING == INTRA_LIBRARY_FONT_LOADING_Dummy
namespace Intra { INTRA_BEGIN

namespace FontLoadingAPI {

struct Font {};

//TODO: заменить заглушку, чтобы она выдавала квадратики

FontHandle FontCreate(StringView name, unsigned height, unsigned* yadvance)
{
	(void)name;
	return FontCreateFromMemory(nullptr, 0, height, yadvance);
}

FontHandle FontCreateFromMemory(const void* data, size_t length, unsigned height, unsigned* yadvance)
{
	(void)data; (void)length; (void)height;
	if(yadvance) *yadvance=1;
	static Font font;
	return &font;
}

void FontDelete(FontHandle font) {(void)font;}

const byte* FontGetCharBitmap(FontHandle font, int code, I16Vec2* offset, U16Vec2* size)
{
	(void)font; (void)code;
	*offset = {0, 0};
	*size = {1, 1};
	static const byte whitePixel=255;
	return &whitePixel;
}

void FontGetCharMetrics(FontHandle font, int code, short* xadvance, short* leftSideBearing)
{
	(void)font; (void)code;
	if(leftSideBearing!=nullptr) *leftSideBearing = 1;
	if(xadvance!=nullptr) *xadvance = 1;
}

short FontGetKerning(FontHandle font, int left, int right)
{
	(void)font; (void)left; (void)right;
	return 0;
}

}
} INTRA_END

#elif(INTRA_LIBRARY_FONT_LOADING == INTRA_LIBRARY_FONT_LOADING_STB)

//See FontLoading_STB.cxx

#elif(INTRA_LIBRARY_FONT_LOADING == INTRA_LIBRARY_FONT_LOADING_FreeType)

INTRA_PUSH_DISABLE_ALL_WARNINGS

#include <ft2build.h>
#include FT_FREETYPE_H

INTRA_WARNING_POP

namespace Intra { INTRA_BEGIN

namespace FontLoadingAPI {

static FT_Library ft=nullptr;

static struct Deinitor
{
	~Deinitor()
	{
		if(ft != nullptr) FT_Done_FreeType(ft);
	}
} Deinitor; //Деинициализация при выходе из программы

struct Font
{
	FT_Face face;
	void* dataCopy;
	size_t dataSize;
};

FontHandle FontCreate(StringView name, uint16 height)
{
	if(ft==nullptr) FT_Init_FreeType(&ft);
	FontHandle desc = new Font;
	desc->dataCopy = nullptr;
	desc->dataSize = 0;
	if(FT_New_Face(ft, String(name).CStr(), 0, &desc->face)!=0) return nullptr;
	FT_Set_Pixel_Sizes(desc->face, height, height);
	return desc;
}

FontHandle FontCreateFromMemory(const void* data, size_t length, uint16 height)
{
	if(ft==nullptr) FT_Init_FreeType(&ft);
	FontHandle desc = new Font;
	size_t bytesToAllocate = length;
	desc->dataCopy = GlobalHeap.Allocate(bytesToAllocate, INTRA_SOURCE_INFO);
	desc->dataSize = bytesToAllocate;
	z_D::memcpy(desc->dataCopy, data, length);
	FT_New_Memory_Face(ft, reinterpret_cast<const FT_Byte*>(desc->dataCopy), long(length), 0, &desc->face);
	FT_Set_Pixel_Sizes(desc->face, height, height);
	return desc;
}

void FontDelete(FontHandle font)
{
	if(font==nullptr) return;
	FT_Done_Face(font->face);
	GlobalHeap.Free(font->dataCopy, font->dataSize);
	delete font;
}

const byte* FontGetCharBitmap(FontHandle desc, unsigned code, I16Vec2* offset, U16Vec2* size)
{
	FT_Face face = desc->face;
	FT_Load_Char(face, code, FT_LOAD_RENDER);
	auto glyph = face->glyph;
	*offset = {short(glyph->bitmap_left), short(glyph->bitmap_top)};
	*size = {uint16(glyph->bitmap.width), uint16(glyph->bitmap.rows)};
	return glyph->bitmap.buffer;
}

}
} INTRA_END

#elif(INTRA_LIBRARY_FONT_LOADING == INTRA_LIBRARY_FONT_LOADING_Gdiplus)

using Intra::GLSL::max;
using Intra::GLSL::min;

struct IUnknown;

INTRA_PUSH_DISABLE_ALL_WARNINGS

#include <olectl.h>
#include <gdiplus.h>

//TODO

INTRA_WARNING_POP


using namespace Gdiplus;
using namespace Gdiplus::DllExports;

namespace Intra { INTRA_BEGIN
namespace FontLoadingAPI {

//static FT_Library ft=nullptr;

struct Deinitor {~Deinitor() {/*if(ft!=nullptr) FT_Done_FreeType(ft);}*/}} Deinitor; //Деинициализация при выходе из программы

FontHandle FontCreate(StringView name, unsigned height, unsigned* yAdvance)
{
	(void)name; (void)height; (void)yAdvance;
	/*if(ft==nullptr) FT_Init_FreeType(&ft);
	FT_Face font;
	if(FT_New_Face(ft, name, 0, &font)!=0) throw FileNotFoundException(name);
	FT_Set_Pixel_Sizes(font, height, height);
	return font;*/return nullptr;
}

FontHandle FontCreateFromMemory(const void* data, size_t length, size_t height, unsigned* yadvance)
{
	(void)data; (void)length; (void)height; (void)yadvance;
	/*if(ft==nullptr) FT_Init_FreeType(&ft);
	FT_Face font;
	FT_New_Memory_Face(ft, (const FT_Byte*)data, length, 0, &font);
	FT_Set_Pixel_Sizes(font, height, height);
	return (handle)font;*/ return nullptr;
}

void FontDelete(FontHandle font) {(void)font;/*if(font.ptr!=nullptr) FT_Done_Face((FT_Face)font.ptr);*/}

const byte* FontGetCharBitmap(FontHandle font, int code, I16Vec2* oOffset, U16Vec2* oSize)
{
	(void)font; (void)code;
	static const byte whitePixel = 255;
	/*FT_Load_Char((FT_Face)font.ptr, code, FT_LOAD_RENDER);
	auto glyph=((FT_Face)font.ptr)->glyph;
	*offset=spoint2((short)glyph->bitmap_left, (short)glyph->bitmap_top);
	*size=ussize2((uint16)glyph->bitmap.width, (uint16)glyph->bitmap.rows);
	return glyph->bitmap.buffer;*/
	*oOffset = I16Vec2(0,0);
	*oSize = U16Vec2(1,1);
	return &whitePixel;
}

}
} INTRA_END

#elif(INTRA_LIBRARY_FONT_LOADING==INTRA_LIBRARY_FONT_LOADING_Qt)

#error "INTRA_LIBRARY_FONT_LOADING_Qt is not implemented!"

#elif(INTRA_LIBRARY_FONT_LOADING==INTRA_LIBRARY_FONT_LOADING_SDL)

#error "INTRA_LIBRARY_FONT_LOADING_SDL is not implemented!"

#endif
