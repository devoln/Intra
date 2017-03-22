#pragma once

#ifndef INTRA_NO_DDS_LOADER

#include "Loader.h"
#include "Platform/CppWarnings.h"
#include "Range/Polymorphic/OutputRange.h"

namespace Intra {

namespace Image {

INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS

class LoaderDDS: public AImageLoader
{
	LoaderDDS() {}
public:
	ImageInfo GetInfo(InputStream stream) const override;
	AnyImage Load(InputStream stream) const override;
	void Save(const AnyImage& img, OutputStream& stream) const;
	bool IsValidHeader(const void* header, size_t headerSize) const override;
	FileFormat FileFormatOfLoader() const override {return FileFormat::DDS;}

	static const LoaderDDS Instance;
};

INTRA_WARNING_POP

}}

#endif
