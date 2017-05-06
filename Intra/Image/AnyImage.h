#pragma once

#include "Cpp/Warnings.h"
#include "Cpp/Fundamental.h"
#include "Image/ImageFormat.h"
#include "Image/ImageInfo.h"
#include "Math/Vector.h"
#include "Container/Sequential/Array.h"
#include "Range/Polymorphic/InputRange.h"
#include "Range/Polymorphic/ForwardRange.h"

namespace Intra { namespace Image {

enum class FileFormat: byte;

INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS

class AnyImage
{
public:
	AnyImage(null_t=null):
		Data(), Info(), SwapRB(false), LineAlignment(1) {}

	AnyImage(Math::USVec3 size, ImageFormat format, ushort mipmapCount=0, ImageType type=ImageType_2D):
		Data(), Info(size, format, type, mipmapCount), SwapRB(false), LineAlignment(1) {}

	bool operator==(null_t) const {return Data==null;}
	bool operator!=(null_t) const {return !operator==(null);}

	AnyImage ExtractChannel(char channelName, ImageFormat compatibleFormat, ushort newLineAlignment=0) const;

	static AnyImage FromData(Math::USVec3 size, ImageFormat format, ImageType type, const void* data,
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

	Array<const void*> GetMipmapPointers() const;

#ifndef INTRA_NO_IMAGE_LOADING
	static FileFormat DetectFileFormatByHeader(byte header[12]);
	static ImageInfo GetImageInfo(ForwardStream stream, FileFormat* oFormat=null);

	static AnyImage FromStream(ForwardStream stream);

#endif
};

INTRA_WARNING_POP

}

using Image::AnyImage;

}
