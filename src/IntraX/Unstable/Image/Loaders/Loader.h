#pragma once

#include "Intra/GloballyRegistered.h"
#include "IntraX/Unstable/Image/ImageInfo.h"
#include "Intra/Range/ListRange.h"

#include "Intra/Range/Polymorphic/InputRange.h"

namespace Intra { INTRA_BEGIN
INTRA_IGNORE_WARN_NO_VIRTUAL_DESTRUCTOR
INTRA_IGNORE_WARN_COPY_MOVE_IMPLICITLY_DELETED
class AnyImage;
enum class FileFormat: byte {JPEG, BMP, GIF, TIFF, DDS, PNG, TGA, KTX, Unknown};

class AImageLoader: public GloballyRegistered<AImageLoader>
{
public:
	virtual ImageInfo GetInfo(IInputStream& stream) const = 0;
	virtual AnyImage Load(IInputStream& stream) const = 0;
	virtual bool IsValidHeader(const void* header, size_t headerSize) const = 0;
	virtual FileFormat FileFormatOfLoader() const = 0;
};

class LoaderDDS: public AImageLoader
{
public:
	ImageInfo GetInfo(IInputStream& stream) const override;
	AnyImage Load(IInputStream& stream) const override;
	void Save(const AnyImage& img, IOutputStream& stream) const;
	bool IsValidHeader(const void* header, size_t headerSize) const override;
	FileFormat FileFormatOfLoader() const override {return FileFormat::DDS;}

#ifndef INTRA_NO_DDS_LOADER
	static const LoaderDDS Instance;
#endif
};

class LoaderKTX: public AImageLoader
{
public:
	ImageInfo GetInfo(IInputStream& stream) const override;
	AnyImage Load(IInputStream& stream) const override;
	void Save(const AnyImage& img, IOutputStream& stream) const;
	bool IsValidHeader(const void* header, size_t headerSize) const override;
	FileFormat FileFormatOfLoader() const override { return FileFormat::KTX; }

#ifndef INTRA_NO_KTX_LOADER
	static const LoaderKTX Instance;
#endif
};

class LoaderTGA: public AImageLoader
{
public:
	ImageInfo GetInfo(IInputStream& stream) const override;
	AnyImage Load(IInputStream& stream) const override;
	bool IsValidHeader(const void* header, size_t headerSize) const override;
	FileFormat FileFormatOfLoader() const override { return FileFormat::TGA; }

#ifndef INTRA_NO_TGA_LOADER
	static const LoaderTGA Instance;
#endif
};

class LoaderBMP: public AImageLoader
{
public:
	ImageInfo GetInfo(IInputStream& stream) const override;
	AnyImage Load(IInputStream& stream) const override;
	bool IsValidHeader(const void* header, size_t headerSize) const override;
	FileFormat FileFormatOfLoader() const override { return FileFormat::BMP; }

#ifndef INTRA_NO_BMP_LOADER
	static const LoaderBMP Instance;
#endif
};


class LoaderGIF: public AImageLoader
{
	LoaderGIF() {}
public:
	ImageInfo GetInfo(IInputStream& stream) const override;
	AnyImage Load(IInputStream& stream) const override;
	bool IsValidHeader(const void* header, size_t headerSize) const override;
	FileFormat FileFormatOfLoader() const override {return FileFormat::GIF;}

#ifndef INTRA_NO_GIF_LOADER
	//static const LoaderGIF Instance;
#endif
};

class LoaderJPEG: public AImageLoader
{
public:
	ImageInfo GetInfo(IInputStream& stream) const override;
	AnyImage Load(IInputStream& stream) const override;
	bool IsValidHeader(const void* header, size_t headerSize) const override;
	FileFormat FileFormatOfLoader() const override {return FileFormat::JPEG;}

#ifndef INTRA_NO_JPEG_LOADER
	static const LoaderJPEG Instance;
#endif
};

class LoaderPNG: public AImageLoader
{
	LoaderPNG() {}
public:
	ImageInfo GetInfo(IInputStream& stream) const override;
	AnyImage Load(IInputStream& stream) const override;
	bool IsValidHeader(const void* header, size_t headerSize) const override;
	FileFormat FileFormatOfLoader() const override {return FileFormat::PNG;}

#ifndef INTRA_NO_PNG_LOADER
	static const LoaderPNG Instance;
#endif
};

class LoaderTIFF: public AImageLoader
{
public:
	ImageInfo GetInfo(IInputStream& stream) const override;
	AnyImage Load(IInputStream& stream) const override;
	bool IsValidHeader(const void* header, size_t headerSize) const override;
	FileFormat FileFormatOfLoader() const override {return FileFormat::TIFF;}

#ifndef INTRA_NO_TIFF_LOADER
	//static const LoaderTIFF Instance;
#endif
};

} INTRA_END
