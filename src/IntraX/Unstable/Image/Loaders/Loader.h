#pragma once

#include "Intra/GloballyRegistered.h"
#include "IntraX/Unstable/Image/ImageInfo.h"
#include "Intra/Range/ListRange.h"

#include "Intra/Range/Polymorphic/InputRange.h"

INTRA_BEGIN
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
INTRA_END
