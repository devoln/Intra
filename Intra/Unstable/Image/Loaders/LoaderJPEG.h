#pragma once

#ifndef INTRA_NO_JPEG_LOADER

#include "Loader.h"

INTRA_BEGIN
INTRA_WARNING_DISABLE_NO_VIRTUAL_DESTRUCTOR
INTRA_WARNING_DISABLE_COPY_MOVE_IMPLICITLY_DELETED
class LoaderJPEG: public AImageLoader
{
public:
	ImageInfo GetInfo(IInputStream& stream) const override;
	AnyImage Load(IInputStream& stream) const override;
	bool IsValidHeader(const void* header, size_t headerSize) const override;
	FileFormat FileFormatOfLoader() const override {return FileFormat::JPEG;}

	static const LoaderJPEG Instance;
};
INTRA_END

#endif
