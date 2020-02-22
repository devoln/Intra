#pragma once

#include "Math/Vector3.h"

#include "Core/Range/Polymorphic/InputRange.h"
#include "Core/Range/Polymorphic/ForwardRange.h"

#include "Utils/FixedArray.h"
#include "Container/Sequential/Array.h"

#include "ImageFormat.h"
#include "ImageInfo.h"

INTRA_BEGIN
enum class FileFormat: byte;

class AnyImage
{
public:
	AnyImage(null_t=null):
		Data(), Info(), SwapRB(false), LineAlignment(1) {}

	AnyImage(USVec3 size, ImageFormat format, ushort mipmapCount=0, ImageType type=ImageType_2D):
		Data(), Info(size, format, type, mipmapCount), SwapRB(false), LineAlignment(1) {}

	bool operator==(null_t) const {return Data.Empty();}
	bool operator!=(null_t) const {return !operator==(null);}

	AnyImage ExtractChannel(char channelName, ImageFormat compatibleFormat, ushort newLineAlignment=0) const;

	static AnyImage FromData(USVec3 size, ImageFormat format, ImageType type, const void* data,
		ushort borderLeft, ushort borderTop, ushort borderRight, ushort borderBottom);

	Array<byte> Data;
	ImageInfo Info;
	bool SwapRB;
	byte LineAlignment;

	const void* GetMipmapDataPtr(size_t mip) const;

	void* GetMipmapDataPtr(size_t mip)
	{
		return const_cast<void*>( const_cast<const AnyImage*>(this)->GetMipmapDataPtr(mip) );
	}

	FixedArray<const void*> GetMipmapPointers() const;

#ifndef INTRA_NO_IMAGE_LOADING
	static FileFormat DetectFileFormatByHeader(byte header[12]);
	static ImageInfo GetImageInfo(ForwardStream stream, FileFormat* oFormat = null);

	static AnyImage FromStream(ForwardStream stream);

#endif
};
INTRA_END
