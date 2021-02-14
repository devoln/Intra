#pragma once

#include "Intra/Range/Polymorphic/InputRange.h"
#include "Intra/Range/Polymorphic/ForwardRange.h"

#include "IntraX/Container/Sequential/Array.h"
#include "IntraX/Math/Vector3.h"
#include "IntraX/Utils/FixedArray.h"

#include "ImageFormat.h"
#include "ImageInfo.h"

namespace Intra { INTRA_BEGIN
enum class FileFormat: byte;

class AnyImage
{
public:
	AnyImage() = default;

	AnyImage(U16Vec3 size, ImageFormat format, NonNegative<short> mipmapCount = 0, ImageType type = ImageType_2D):
		Data(), Info(size, format, type, mipmapCount), SwapRB(false), LineAlignment(1) {}

	bool Empty() const {return Data.Empty();}

	AnyImage ExtractChannel(char channelName, ImageFormat compatibleFormat, uint16 newLineAlignment=0) const;

	static AnyImage FromData(U16Vec3 size, ImageFormat format, ImageType type, const void* data,
		uint16 borderLeft, uint16 borderTop, uint16 borderRight, uint16 borderBottom);

	ArrayList<byte> Data;
	ImageInfo Info;
	bool SwapRB = false;
	byte LineAlignment = 1;

	const void* GetMipmapDataPtr(index_t mip) const;

	void* GetMipmapDataPtr(index_t mip)
	{
		return const_cast<void*>( const_cast<const AnyImage*>(this)->GetMipmapDataPtr(mip) );
	}

	FixedArray<const void*> GetMipmapPointers() const;

#ifndef INTRA_NO_IMAGE_LOADING
	static FileFormat DetectFileFormatByHeader(byte header[12]);
	static ImageInfo GetImageInfo(ForwardStream stream, Optional<FileFormat&> oFormat = {});

	static AnyImage FromStream(ForwardStream stream);

#endif
};
} INTRA_END
