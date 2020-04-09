#include "Extra/Unstable/Image/AnyImage.h"

#include "Extra/Container/Sequential/Array.h"
#include "Intra/Range/Mutation/Fill.h"
#include "Intra/Range/ListRange.h"
#include "Extra/Unstable/Image/Loaders/Loader.h"

INTRA_BEGIN
AnyImage AnyImage::FromData(USVec3 size, ImageFormat format, ImageType type,
	const void* data, uint16 borderLeft, uint16 borderTop, uint16 borderRight, uint16 borderBottom)
{
	INTRA_PRECONDITION(data != null);
	INTRA_PRECONDITION(size.x * size.y * size.z != 0);
	INTRA_PRECONDITION(format != ImageFormat::End);
	INTRA_PRECONDITION(type < ImageType_End);

	AnyImage result({size.x + borderLeft + borderRight, size.y + borderTop + borderBottom, 1}, format, 0, type);
	result.Data.SetCountUninitialized(index_t(result.Info.CalculateFullDataSize(1)));
	if(data == null || borderLeft + borderRight + borderTop + borderBottom != 0)
		FillZeros(result.Data);

	result.LineAlignment = 1;
	for(int y = borderTop; y < size.y + borderTop; y++)
	{
		const int dstOffset = y*result.Info.Size.x + borderLeft;
		const int srcOffset = y*size.x;
		const int bpp = format.BytesPerPixel();
		const int lineSize = size.x*bpp;
		Misc::BitwiseCopyUnsafe(result.Data.Data() + dstOffset*bpp,
			reinterpret_cast<const byte*>(data) + srcOffset*bpp, lineSize);
	}

	return result;
}


#if INTRA_DISABLED
AnyImage AnyImage::ExtractChannel(char channelName, ImageFormat compatibleFormat, uint16 newLineAlignment) const
{
	if(newLineAlignment == 0) newLineAlignment = LineAlignment;
	const auto channelType = Info.Format.GetComponentType();

	INTRA_DEBUG_ASSERT(compatibleFormat.GetValueType() == channelType);
	INTRA_DEBUG_ASSERT(compatibleFormat.ComponentCount() == 1);

	const auto channelWidth = channelType.Size();
	const auto channelCount = Info.Format.ComponentCount();

	uint16 channelIndex = 0;
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


const void* AnyImage::GetMipmapDataPtr(index_t mip) const
{
	const byte* ptr = Data.Data();
	for(index_t i = 0; i < mip; i++)
		ptr += Info.CalculateMipmapDataSize(i, LineAlignment);
	return ptr;
}

FixedArray<const void*> AnyImage::GetMipmapPointers() const
{
	const auto mc = Max<index_t>(Info.MipmapCount, 1);
	FixedArray<const void*> result(mc);
	for(index_t i = 0; i < mc; i++)
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

	for(auto& loader: AImageLoader::Instances())
	{
		if(!loader.IsValidHeader(header, 12)) continue;
		return loader.Load(stream);
	}
	return null;
}

FileFormat AnyImage::DetectFileFormatByHeader(byte header[12])
{
	for(auto& loader: AImageLoader::Instances())
		if(loader.IsValidHeader(header, 12)) return loader.FileFormatOfLoader();
	return FileFormat::Unknown;
}

ImageInfo AnyImage::GetImageInfo(ForwardStream stream, Optional<FileFormat&> oFormat)
{
	if(oFormat) oFormat.Unwrap() = FileFormat::Unknown;
	ImageInfo result;
	for(auto& loader: AImageLoader::Instances())
	{
		result = loader.GetInfo(stream);
		if(result == null) continue;
		if(oFormat) oFormat.Unwrap() = loader.FileFormatOfLoader();
		break;
	}
	return result;
}

#endif

INTRA_END
