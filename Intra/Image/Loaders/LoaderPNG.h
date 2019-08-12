#pragma once

#ifndef INTRA_NO_PNG_LOADER

#include "Loader.h"


INTRA_BEGIN
namespace Image {

INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS

class LoaderPNG: public AImageLoader
{
	LoaderPNG() {}
public:
	ImageInfo GetInfo(IInputStream& stream) const override;
	AnyImage Load(IInputStream& stream) const override;
	bool IsValidHeader(const void* header, size_t headerSize) const override;
	FileFormat FileFormatOfLoader() const override {return FileFormat::PNG;}

	static const LoaderPNG Instance;
};

INTRA_WARNING_POP

}}

#endif
