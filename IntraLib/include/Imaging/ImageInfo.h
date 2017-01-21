#pragma once

#include "Platform/CppWarnings.h"
#include "Core/FundamentalTypes.h"
#include "ImageFormat.h"
#include "Math/Vector.h"

namespace Intra { namespace Imaging {

INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS

enum ImageType: byte {
	ImageType_1D, ImageType_1DArray, ImageType_2D,
	ImageType_2DArray, ImageType_3D, ImageType_Cube, ImageType_CubeArray, ImageType_End
};

enum class CubeFace: byte {PositiveX, NegativeX, PositiveY, NegativeY, PositiveZ, NegativeZ};


Math::USVec3 CalculateMipmapSize(Math::USVec3 size, ImageType type, uint mip);
Math::USVec3 CalculateMipmapOffset(Math::USVec3 offset, ImageType type, uint mip);

struct ImageInfo
{
	ImageInfo(Math::USVec3 size = {0, 0, 0}, ImageFormat format = null, ImageType type = ImageType_End, ushort mipmaps = 0):
		Size(size), MipmapCount(mipmaps), Format(format), Type(type) {}

	bool operator==(const ImageInfo& rhs) const
	{
		return Size==rhs.Size && Format==rhs.Format && Type==rhs.Type &&
			(MipmapCount==rhs.MipmapCount || MipmapCount==0 || rhs.MipmapCount==0);
	}
	bool operator!=(const ImageInfo& rhs) const { return !operator==(rhs); }

	bool operator==(null_t) const { return Size.x*Size.y*Size.z==0 || Format==null || Type>=ImageType_End; }
	bool operator!=(null_t) const { return !operator==(null); }

	Math::USVec3 Size;
	ushort MipmapCount; //=0, если требуется автоматическая генерация всех мип-уровней
	ImageFormat Format;
	ImageType Type;

	ushort CalculateMaxMipmapCount() const;
	Math::USVec3 CalculateMipmapSize(size_t mip) const;
	size_t CalculateMipmapDataSize(size_t mip, size_t lineAlignment) const;
	size_t CalculateFullDataSize(size_t lineAlignment) const;
};

}

INTRA_WARNING_POP

using Imaging::ImageInfo;
using Imaging::CubeFace;

using Imaging::ImageType;
using Imaging::ImageType_1D;
using Imaging::ImageType_1DArray;
using Imaging::ImageType_2D;
using Imaging::ImageType_2DArray;
using Imaging::ImageType_3D;
using Imaging::ImageType_Cube;
using Imaging::ImageType_CubeArray;
using Imaging::ImageType_End;
}
