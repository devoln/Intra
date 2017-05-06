#pragma once

#ifndef INTRA_NO_DDS_LOADER

#include "Loader.h"
#include "Cpp/Warnings.h"

namespace Intra {

namespace IO {class IOutputStream; class IInputStream;}

namespace Image {

INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS

class LoaderKTX: public AImageLoader
{
	LoaderKTX() {}
public:
	ImageInfo GetInfo(InputStream stream) const override;
	AnyImage Load(InputStream stream) const override;
	void Save(const AnyImage& img, IO::IOutputStream& stream) const;
	bool IsValidHeader(const void* header, size_t headerSize) const override;
	FileFormat FileFormatOfLoader() const override {return FileFormat::KTX;}

	static const LoaderKTX Instance;
};

INTRA_WARNING_POP

}}

#endif
