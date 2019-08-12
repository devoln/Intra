#pragma once

#ifndef INTRA_NO_BMP_LOADER

#include "Loader.h"


INTRA_BEGIN
namespace Image {

INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS

class LoaderBMP: public AImageLoader
{
	LoaderBMP() {}
public:
	ImageInfo GetInfo(IInputStream& stream) const override;
	AnyImage Load(IInputStream& stream) const override;
	bool IsValidHeader(const void* header, size_t headerSize) const override;
	FileFormat FileFormatOfLoader() const override {return FileFormat::BMP;}

	static const LoaderBMP Instance;
};

INTRA_WARNING_POP

}}

#endif
