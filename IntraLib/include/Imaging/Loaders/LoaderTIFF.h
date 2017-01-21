#pragma once

#ifndef INTRA_NO_TIFF_LOADER

#include "Loader.h"
#include "Platform/CppWarnings.h"

namespace Intra { namespace Imaging {

INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS

class LoaderTIFF: public AImageLoader
{
	LoaderTIFF() {}
public:
	ImageInfo GetInfo(IO::IInputStream& stream) const override;
	Image Load(IO::IInputStream& stream, size_t bytes) const override;
	bool IsValidHeader(const void* header, size_t headerSize) const override;
	FileFormat FileFormatOfLoader() const override {return FileFormat::TIFF;}

	//static const LoaderGIF Instance;
};

INTRA_WARNING_POP

}}

#endif
