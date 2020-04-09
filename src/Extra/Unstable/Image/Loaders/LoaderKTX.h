#pragma once

#ifndef INTRA_NO_DDS_LOADER

#include "Loader.h"

#include "Intra/Range/Polymorphic/IInput.h"
#include "Intra/Range/Polymorphic/IOutput.h"

INTRA_BEGIN
INTRA_IGNORE_WARNING_NO_VIRTUAL_DESTRUCTOR
INTRA_IGNORE_WARNING_COPY_MOVE_IMPLICITLY_DELETED
class LoaderKTX: public AImageLoader
{
public:
	ImageInfo GetInfo(IInputStream& stream) const override;
	AnyImage Load(IInputStream& stream) const override;
	void Save(const AnyImage& img, IOutputStream& stream) const;
	bool IsValidHeader(const void* header, size_t headerSize) const override;
	FileFormat FileFormatOfLoader() const override {return FileFormat::KTX;}

	static const LoaderKTX Instance;
};
INTRA_END

#endif
