#pragma once

#ifndef INTRA_NO_TGA_LOADER

#include "Loader.h"
#include "Platform/CppWarnings.h"

namespace Intra { namespace Image {

INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS

class LoaderTGA: public AImageLoader
{
	LoaderTGA() {}
public:
	ImageInfo GetInfo(InputStream stream) const override;
	AnyImage Load(InputStream stream) const override;
	bool IsValidHeader(const void* header, size_t headerSize) const override;
	FileFormat FileFormatOfLoader() const override {return FileFormat::TGA;}

	static const LoaderTGA Instance;
};

INTRA_WARNING_POP

}}

#endif
