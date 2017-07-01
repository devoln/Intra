#pragma once

#ifndef INTRA_NO_JPEG_LOADER

#include "Loader.h"
#include "Cpp/Warnings.h"

namespace Intra { namespace Image {

INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS

class LoaderGIF: public AImageLoader
{
	LoaderGIF() {}
public:
	ImageInfo GetInfo(IInputStream& stream) const override;
	AnyImage Load(IInputStream& stream) const override;
	bool IsValidHeader(const void* header, size_t headerSize) const override;
	FileFormat FileFormatOfLoader() const override {return FileFormat::GIF;}

	//static const LoaderGIF Instance;
};

INTRA_WARNING_POP

}}

#endif
