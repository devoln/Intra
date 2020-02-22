#pragma once

#include "ImageFormat.h"

#include "Math/Vector3.h"

INTRA_BEGIN
enum ImageType: byte {
	ImageType_1D, ImageType_1DArray, ImageType_2D,
	ImageType_2DArray, ImageType_3D, ImageType_Cube, ImageType_CubeArray, ImageType_End
};

enum class CubeFace: byte {PositiveX, NegativeX, PositiveY, NegativeY, PositiveZ, NegativeZ};


USVec3 CalculateMipmapSize(USVec3 size, ImageType type, uint mip);
USVec3 CalculateMipmapOffset(USVec3 offset, ImageType type, uint mip);

struct ImageInfo
{
	ImageInfo(USVec3 size = {0, 0, 0}, ImageFormat format = null, ImageType type = ImageType_End, ushort mipmaps = 0):
		Size(size), MipmapCount(mipmaps), Format(format), Type(type) {}

	bool operator==(const ImageInfo& rhs) const
	{
		return Size == rhs.Size && Format == rhs.Format && Type == rhs.Type &&
			(MipmapCount == rhs.MipmapCount || MipmapCount == 0 || rhs.MipmapCount == 0);
	}
	bool operator!=(const ImageInfo& rhs) const {return !operator==(rhs);}

	bool operator==(null_t) const {return Size.x*Size.y*Size.z == 0 || Format == null || Type >= ImageType_End;}
	bool operator!=(null_t) const {return !operator==(null);}

	USVec3 Size;
	ushort MipmapCount; //=0, если требуется автоматическая генерация всех мип-уровней
	ImageFormat Format;
	ImageType Type;

	ushort CalculateMaxMipmapCount() const;
	USVec3 CalculateMipmapSize(size_t mip) const;
	size_t CalculateMipmapDataSize(size_t mip, size_t lineAlignment) const;
	size_t CalculateFullDataSize(size_t lineAlignment) const;
};
StringView ToString(ImageType t);
INTRA_END
