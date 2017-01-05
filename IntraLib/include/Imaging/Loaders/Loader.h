#pragma once

#include "Imaging/ImageInfo.h"
#include "Range/ListRange.h"
#include "Platform/CppWarnings.h"

namespace Intra {

namespace IO {class IInputStream;}

namespace Imaging {

class Image;

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

	virtual ImageInfo GetInfo(IO::IInputStream& stream) const = 0;
	virtual Image Load(IO::IInputStream& stream, size_t bytes) const = 0;
	virtual bool IsValidHeader(const void* header, size_t headerSize) const = 0;
	virtual FileFormat FileFormatOfLoader() const = 0;

	static forceinline FListRange<AImageLoader, AImageLoader> GetRegisteredLoaders()
	{return FListRange<AImageLoader, AImageLoader>(firstLoader);}
};

INTRA_WARNING_POP

}}
