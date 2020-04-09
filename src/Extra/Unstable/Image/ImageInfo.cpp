#include "Extra/Unstable/Image/ImageInfo.h"

#include "Intra/Range/StringView.h"

#include "Extra/Container/Sequential/String.h"

#include "Intra/Math/Math.h"
#include "Extra/Math/Vector3.h"

INTRA_BEGIN
StringView ToString(ImageType t)
{
	INTRA_PRECONDITION(t < ImageType_End);
	static const StringView values[] = {"1D", "1DArray", "2D", "2DArray", "3D", "Cube", "CubeArray", "Invalid"};
	INTRA_CHECK_TABLE_SIZE(values, ImageType_End+1);
	return values[byte(t)];
}

short ImageInfo::CalculateMaxMipmapCount() const
{
	uint16 maxDimension = Max(Size.x, Size.y);
	if(Type != ImageType_2DArray) maxDimension = Max(maxDimension, Size.z);
	const short maxUncompressedLevel = maxDimension==1? 0: Log2i(Max<unsigned>(1u, maxDimension));
	const short numLevels = short(maxUncompressedLevel+1);
	return Max(numLevels, short(0));
}

USVec3 CalculateMipmapSize(USVec3 size, ImageType type, index_t mip)
{
	USVec3 result;
	result.z = size.z;
	const uint16 dims = uint16(2+(type == ImageType_3D));
	for(uint16 i = 0; i<dims; i++)
		result[i] = Max(uint16(1), uint16(size[i] >> mip));
	return result;
}

USVec3 CalculateMipmapOffset(USVec3 offset, ImageType type, index_t mip)
{
	USVec3 result = CalculateMipmapSize(offset, type, mip);
	return Min(result, USVec3(offset));
}

USVec3 ImageInfo::CalculateMipmapSize(index_t mip) const
{
	if(mip>=MipmapCount && mip!=0) return {0, 0, 0};
	return Intra::CalculateMipmapSize(Size, Type, mip);
}

size_t ImageInfo::CalculateMipmapDataSize(index_t mip, size_t lineAlignment) const
{
	USVec3 sz = CalculateMipmapSize(mip);
	const uint16 dims = uint16(2 + (Type == ImageType_3D));
	size_t alignmentBytes = 0;
	const unsigned bpp = Format.BitsPerPixel();
	if(!Format.IsCompressed())
		alignmentBytes = lineAlignment - 1 - (sz.x*bpp/8 + lineAlignment - 1) % lineAlignment;
	else for(unsigned k = 0; k<dims; k++)
		if(sz[k] % 4 != 0) sz[k] = uint16((sz[k] / 4 + 1)*4);
	return (sz.x*bpp/8+alignmentBytes)*sz.y*sz.z;
}

size_t ImageInfo::CalculateFullDataSize(size_t lineAlignment) const
{
	size_t size = 0;
	for(index_t i = 0, mips = MipmapCount == 0? 1: MipmapCount; i < mips; i++)
		size += CalculateMipmapDataSize(i, lineAlignment);
	return size;
}
INTRA_END
