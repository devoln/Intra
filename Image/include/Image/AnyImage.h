﻿#pragma once

#include "Platform/CppWarnings.h"
#include "Platform/FundamentalTypes.h"
#include "IO/Stream.h"
#include "Image/ImageFormat.h"
#include "Image/ImageInfo.h"
#include "Math/Vector.h"
#include "Containers/Array.h"

namespace Intra { namespace Image {

enum class FileFormat: byte;

INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS

class AnyImage
{
public:
	AnyImage(null_t=null):
		Data(), Info(), SwapRB(false), LineAlignment(1) {}

	AnyImage(const AnyImage& rhs) = default;

	AnyImage(AnyImage&& rhs):
		Data(Meta::Move(rhs.Data)), Info(rhs.Info),
		SwapRB(rhs.SwapRB), LineAlignment(rhs.LineAlignment) {}

	AnyImage(Math::USVec3 size, ImageFormat format, ushort mipmapCount=0, ImageType type=ImageType_2D):
		Data(), Info(size, format, type, mipmapCount), SwapRB(false), LineAlignment(1) {}

	AnyImage& operator=(const AnyImage& rhs) = default;

	AnyImage& operator=(AnyImage&& rhs)
	{
		Data = Meta::Move(rhs.Data);
		Info = rhs.Info;
		SwapRB=  rhs.SwapRB;
		LineAlignment = rhs.LineAlignment;
		return *this;
	}

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
	static ImageInfo GetImageInfo(IO::IInputStream& stream, FileFormat* format=null);
	static ImageInfo GetImageInfo(StringView filename, FileFormat* format=null);

	static AnyImage FromFile(StringView filename);

	static AnyImage FromStream(IO::IInputStream& stream, size_t bytes);

#endif
};

INTRA_WARNING_POP

}

using Image::AnyImage;

}