#include "Imaging/ImageInfo.h"
#include "Range/StringView.h"
#include "Containers/String.h"
#include "Math/MathEx.h"
#include "Math/Vector.h"
#include "Platform/CppWarnings.h"

namespace Intra { namespace Imaging {

INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS

StringView ToString(ImageType t)
{
	INTRA_ASSERT(t<ImageType_End);
	static const StringView values[] = {"1D", "1DArray", "2D", "2DArray", "3D", "Cube", "CubeArray"};
	INTRA_CHECK_TABLE_SIZE(values, ImageType_End);
	return values[byte(t)];
}

ushort ImageInfo::CalculateMaxMipmapCount() const
{
	ushort maxDimension = Math::Max(Size.x, Size.y);
	if(Type!=ImageType_2DArray) maxDimension = Math::Max(maxDimension, Size.z);
	const short maxUncompressedLevel = maxDimension==1? 0: Math::Log2i(Math::Max<uint>(1u, maxDimension));
	ushort numLevels = ushort(maxUncompressedLevel+1);
	return Math::Max(numLevels, ushort(0));
}

Math::USVec3 CalculateMipmapSize(Math::USVec3 size, ImageType type, size_t mip)
{
	Math::USVec3 result;
	result.z = size.z;
	const ushort dims = ushort(2+(type==ImageType_3D));
	for(ushort i = 0; i<dims; i++)
		result[i] = Math::Max(ushort(1), ushort(size[i] >> mip));
	return result;
}

Math::USVec3 CalculateMipmapOffset(Math::USVec3 offset, ImageType type, size_t mip)
{
	Math::USVec3 result = CalculateMipmapSize(offset, type, mip);
	return Math::Min(result, Math::USVec3(offset));
}

Math::USVec3 ImageInfo::CalculateMipmapSize(size_t mip) const
{
	if(mip>=MipmapCount && mip!=0) return {0, 0, 0};
	return Imaging::CalculateMipmapSize(Size, Type, mip);
}

size_t ImageInfo::CalculateMipmapDataSize(size_t mip, size_t lineAlignment) const
{
	Math::USVec3 sz = CalculateMipmapSize(mip);
	const ushort dims = ushort(2+(Type==ImageType_3D));
	size_t alignmentBytes = 0;
	const uint bpp = Format.BitsPerPixel();
	if(!Format.IsCompressed())
		alignmentBytes = lineAlignment-1-(sz.x*bpp/8+lineAlignment-1)%lineAlignment;
	else for(uint k = 0; k<dims; k++)
		if(sz[k]%4!=0) sz[k] = ushort((sz[k]/4+1)*4);
	return (sz.x*bpp/8+alignmentBytes)*sz.y*sz.z;
}

size_t ImageInfo::CalculateFullDataSize(size_t lineAlignment) const
{
	size_t size = 0;
	for(size_t i = 0, mips = MipmapCount==0? 1u: MipmapCount; i<mips; i++)
		size += CalculateMipmapDataSize(i, lineAlignment);
	return size;
}

INTRA_WARNING_POP

}}
