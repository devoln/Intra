#pragma once

#ifndef INTRA_NO_DDS_LOADER

#include "Loader.h"
#include "Cpp/Warnings.h"
#include "Concepts/IInput.h"
#include "Concepts/IOutput.h"

namespace Intra {

namespace Image {

INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS

class LoaderDDS: public AImageLoader
{
	LoaderDDS() {}
public:
	ImageInfo GetInfo(IInputStream& stream) const override;
	AnyImage Load(IInputStream& stream) const override;
	void Save(const AnyImage& img, IOutputStream& stream) const;
	bool IsValidHeader(const void* header, size_t headerSize) const override;
	FileFormat FileFormatOfLoader() const override {return FileFormat::DDS;}

	static const LoaderDDS Instance;
};

INTRA_WARNING_POP

}}

#endif
