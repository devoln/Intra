#pragma once

#include "Core/GloballyRegistered.h"
#include "Image/ImageInfo.h"
#include "Core/Range/ListRange.h"

#include "Core/Range/Polymorphic/InputRange.h"

INTRA_BEGIN
INTRA_WARNING_DISABLE_NO_VIRTUAL_DESTRUCTOR
INTRA_WARNING_DISABLE_COPY_MOVE_IMPLICITLY_DELETED
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
INTRA_END
