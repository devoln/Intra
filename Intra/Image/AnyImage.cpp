#include "Image/AnyImage.h"

#include "Container/Sequential/Array.h"
#include "Core/Range/Mutation/Fill.h"
#include "Core/Range/Generators/ListRange.h"
#include "Image/Loaders/Loader.h"

INTRA_BEGIN
namespace Image {

INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS

using namespace Math;


AnyImage AnyImage::FromData(USVec3 size, ImageFormat format, ImageType type,
	const void* data, ushort borderLeft, ushort borderTop, ushort borderRight, ushort borderBottom)
{
	if(data == null || size.x*size.y*size.z == 0 || format == ImageFormat::End || type == ImageType_End)
		return null; //Переданы неверные параметры!

	AnyImage result({size.x+borderLeft+borderRight, size.y+borderTop+borderBottom, 1}, format, 0, type);
	result.Data.SetCountUninitialized(result.Info.CalculateFullDataSize(1));
	if(data==null || borderLeft+borderRight+borderTop+borderBottom!=0)
		Range::FillZeros(result.Data);

	result.LineAlignment = 1;
	for(uint y = borderTop; y < uint(size.y + borderTop); y++)
	{
		const uint dstOffset = y*result.Info.Size.x+borderLeft;
		const uint srcOffset = y*size.x;
		const uint bpp = format.BytesPerPixel();
		const uint lineSize = size.x*bpp;
		C::memcpy(result.Data.Data() + dstOffset*bpp,
			reinterpret_cast<const byte*>(data) + srcOffset*bpp, lineSize);
	}

	return result;
}


#if INTRA_DISABLED

AnyImage AnyImage::ExtractChannel(char channelName, ImageFormat compatibleFormat, ushort newLineAlignment) const
{
	if(newLineAlignment == 0) newLineAlignment = LineAlignment;
	const auto channelType = Info.Format.GetComponentType();

	INTRA_DEBUG_ASSERT(compatibleFormat.GetValueType()==channelType);
	INTRA_DEBUG_ASSERT(compatibleFormat.ComponentCount()==1);

	const auto channelWidth = channelType.Size();
	const auto channelCount = Info.Format.ComponentCount();

	ushort channelIndex = 0;
	if(channelName=='r' || channelName=='x') channelIndex = 0;
	else if(channelName=='g' || channelName=='y') channelIndex = 1;
	else if(channelName=='b' || channelName=='z') channelIndex = 2;
	else if(channelName=='a' || channelName=='w') channelIndex = 3;
	else return null;

	const size_t usefulSrcLineBytes = size_t(Info.Size.x*Info.Format.BytesPerPixel());
	const size_t usefulDstLineBytes = size_t(Info.Size.x*compatibleFormat.BytesPerPixel());
	const size_t srcLineBytes = (usefulSrcLineBytes+LineAlignment-1)&~(LineAlignment-1);
	const size_t dstLineBytes = (usefulDstLineBytes+newLineAlignment-1)&~(newLineAlignment-1);
	const size_t srcDataSize = Info.Size.y*srcLineBytes;
	const size_t dstDataSize = Info.Size.y*dstLineBytes;

	AnyImage result(Info.Size, compatibleFormat, Info.MipmapCount, Info.Type);
	result.Data.SetCountUninitialized(dstDataSize);

	INTRA_INTERNAL_ERROR("Not implemented!");

	return result;
}

#endif


const void* AnyImage::GetMipmapDataPtr(size_t mip) const
{
	const byte* ptr = Data.Data();
	for(size_t i = 0; i<mip; i++)
		ptr += Info.CalculateMipmapDataSize(i, LineAlignment);
	return ptr;
}

FixedArray<const void*> AnyImage::GetMipmapPointers() const
{
	ushort mc = Max<ushort>(Info.MipmapCount, ushort(1));
	FixedArray<const void*> result(mc);
	for(ushort i = 0; i<mc; i++)
		result[i] = GetMipmapDataPtr(i);
	return result;
}

#ifndef INTRA_NO_IMAGE_LOADING

AnyImage AnyImage::FromStream(ForwardStream stream)
{
	auto oldStream = stream;
	byte header[12];
	stream.RawReadTo(header, sizeof(header));
	stream = Move(oldStream);

	for(auto& loader: AImageLoader::GetRegisteredLoaders())
	{
		if(!loader.IsValidHeader(header, 12)) continue;
		return loader.Load(stream);
	}
	return null;
}

FileFormat AnyImage::DetectFileFormatByHeader(byte header[12])
{
	for(auto& loader: AImageLoader::GetRegisteredLoaders())
		if(loader.IsValidHeader(header, 12)) return loader.FileFormatOfLoader();
	return FileFormat::Unknown;
}

ImageInfo AnyImage::GetImageInfo(ForwardStream stream, FileFormat* oFormat)
{
	if(oFormat) *oFormat = FileFormat::Unknown;
	ImageInfo result;
	for(auto& loader: AImageLoader::GetRegisteredLoaders())
	{
		result = loader.GetInfo(stream);
		if(result == null) continue;
		if(oFormat) *oFormat = loader.FileFormatOfLoader();
		break;
	}
	return result;
}

#endif

INTRA_WARNING_POP

}}
