#pragma once

#include "Font/FontLoadingDeclarations.h"
#include "Math/Vector2.h"
#include "Utils/StringView.h"

namespace Intra { namespace FontLoadingAPI {

	FontHandle FontCreate(StringView name, uint height, uint* yadvance);
	FontHandle FontCreateFromMemory(const void* data, size_t length, uint height, uint* yadvance);
	void FontDelete(FontHandle font);
	const byte* FontGetCharBitmap(FontHandle font, int code, Math::SVec2* offset, Math::USVec2* size);
	void FontGetCharMetrics(FontHandle font, int code, short* xadvance, short* leftSideBearing);
	short FontGetKerning(FontHandle font, int left, int right);

}}
