#pragma once

#ifndef INTRA_NO_DDS_LOADER

#include "Loader.h"

#include "Utils/IInput.h"
#include "Utils/IOutput.h"

INTRA_BEGIN
INTRA_WARNING_DISABLE_NO_VIRTUAL_DESTRUCTOR
INTRA_WARNING_DISABLE_COPY_MOVE_IMPLICITLY_DELETED
class LoaderDDS: public AImageLoader
{
	LoaderDDS() = default;
	LoaderDDS(const LoaderDDS&) = delete;
	LoaderDDS(LoaderDDS&&) = delete;
	LoaderDDS& operator=(const LoaderDDS&) = delete;
	LoaderDDS& operator=(LoaderDDS&&) = delete;
public:
	ImageInfo GetInfo(IInputStream& stream) const override;
	AnyImage Load(IInputStream& stream) const override;
	void Save(const AnyImage& img, IOutputStream& stream) const;
	bool IsValidHeader(const void* header, size_t headerSize) const override;
	FileFormat FileFormatOfLoader() const override {return FileFormat::DDS;}

	static const LoaderDDS Instance;
};
INTRA_END

#endif
