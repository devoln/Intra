#pragma once

#ifndef INTRA_NO_TIFF_LOADER

#include "Loader.h"
#include "Cpp/Warnings.h"

namespace Intra { namespace Image {

INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS

class LoaderTIFF: public AImageLoader
{
	LoaderTIFF() {}
public:
	ImageInfo GetInfo(IInputStream& stream) const override;
	AnyImage Load(IInputStream& stream) const override;
	bool IsValidHeader(const void* header, size_t headerSize) const override;
	FileFormat FileFormatOfLoader() const override {return FileFormat::TIFF;}

	//static const LoaderGIF Instance;
};

INTRA_WARNING_POP

}}

#endif
