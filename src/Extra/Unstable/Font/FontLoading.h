#pragma once

#include "Extra/Unstable/Font/FontLoadingDeclarations.h"
#include "Extra/Math/Vector2.h"
#include "Intra/Range/StringView.h"

INTRA_BEGIN
namespace FontLoadingAPI {
	FontHandle FontCreate(StringView name, unsigned height, unsigned* yadvance);
	FontHandle FontCreateFromMemory(const void* data, size_t length, unsigned height, unsigned* yadvance);
	void FontDelete(FontHandle font);
	const byte* FontGetCharBitmap(FontHandle font, int code, SVec2* offset, USVec2* size);
	void FontGetCharMetrics(FontHandle font, int code, short* xadvance, short* leftSideBearing);
	short FontGetKerning(FontHandle font, int left, int right);

}
INTRA_END
