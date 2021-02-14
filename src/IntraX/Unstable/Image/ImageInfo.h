#pragma once

#include "ImageFormat.h"

#include "IntraX/Math/Vector3.h"

namespace Intra { INTRA_BEGIN
enum ImageType: byte {
	ImageType_1D, ImageType_1DArray, ImageType_2D,
	ImageType_2DArray, ImageType_3D, ImageType_Cube, ImageType_CubeArray, ImageType_End
};

enum class CubeFace: byte {PositiveX, NegativeX, PositiveY, NegativeY, PositiveZ, NegativeZ};


U16Vec3 CalculateMipmapSize(U16Vec3 size, ImageType type, index_t mip);
U16Vec3 CalculateMipmapOffset(U16Vec3 offset, ImageType type, index_t mip);

struct ImageInfo
{
	ImageInfo(U16Vec3 size = {0, 0, 0}, ImageFormat format = nullptr, ImageType type = ImageType_End, NonNegative<short> mipmaps = 0):
		Size(size), MipmapCount(mipmaps), Format(format), Type(type) {}

	bool operator==(const ImageInfo& rhs) const
	{
		return Size == rhs.Size && Format == rhs.Format && Type == rhs.Type &&
			(MipmapCount == rhs.MipmapCount || MipmapCount == 0 || rhs.MipmapCount == 0);
	}
	bool operator!=(const ImageInfo& rhs) const {return !operator==(rhs);}

	bool operator==(decltype(nullptr)) const {return Size.x*Size.y*Size.z == 0 || Format == nullptr || Type >= ImageType_End;}
	bool operator!=(decltype(nullptr)) const {return !operator==(nullptr);}

	U16Vec3 Size;
	short MipmapCount; //=0, если требуется автоматическая генерация всех мип-уровней
	ImageFormat Format;
	ImageType Type;

	short CalculateMaxMipmapCount() const;
	U16Vec3 CalculateMipmapSize(index_t mip) const;
	size_t CalculateMipmapDataSize(index_t mip, size_t lineAlignment) const;
	size_t CalculateFullDataSize(size_t lineAlignment) const;
};
StringView ToString(ImageType t);
} INTRA_END
