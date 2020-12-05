#pragma once

#ifndef INTRA_NO_BMP_LOADER

#include "Loader.h"

INTRA_BEGIN
INTRA_IGNORE_WARN_NO_VIRTUAL_DESTRUCTOR
INTRA_IGNORE_WARN_COPY_MOVE_IMPLICITLY_DELETED
class LoaderBMP: public AImageLoader
{
public:
	ImageInfo GetInfo(IInputStream& stream) const override;
	AnyImage Load(IInputStream& stream) const override;
	bool IsValidHeader(const void* header, size_t headerSize) const override;
	FileFormat FileFormatOfLoader() const override {return FileFormat::BMP;}

	static const LoaderBMP Instance;
};
INTRA_END

#endif
