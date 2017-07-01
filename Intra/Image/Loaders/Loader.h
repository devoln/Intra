#pragma once

#include "Image/ImageInfo.h"
#include "Range/Generators/ListRange.h"
#include "Cpp/Warnings.h"
#include "Range/Polymorphic/InputRange.h"

namespace Intra {

namespace Image {

class AnyImage;

INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS

enum class FileFormat: byte {JPEG, BMP, GIF, TIFF, DDS, PNG, TGA, KTX, Unknown};

class AImageLoader
{
	AImageLoader* mNextLoader;
	static AImageLoader* firstLoader;
protected:
	AImageLoader(): mNextLoader(firstLoader) {firstLoader = this;}
	virtual ~AImageLoader() {}
public:
	AImageLoader* NextListNode() const {return mNextLoader;}

	virtual ImageInfo GetInfo(IInputStream& stream) const = 0;
	virtual AnyImage Load(IInputStream& stream) const = 0;
	virtual bool IsValidHeader(const void* header, size_t headerSize) const = 0;
	virtual FileFormat FileFormatOfLoader() const = 0;

	static forceinline FListRange<AImageLoader, AImageLoader> GetRegisteredLoaders()
	{return FListRange<AImageLoader, AImageLoader>(firstLoader);}
};

INTRA_WARNING_POP

}}
