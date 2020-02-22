#ifndef INTRA_NO_KTX_LOADER

#include "Image/Loaders/LoaderKTX.h"

#include "Image/AnyImage.h"
#include "Image/Bindings/GLenumFormats.h"

INTRA_BEGIN
struct KtxHeader
{
	uint glType, glTypeSize;
	uint glFormat, glInternalFormat, glBaseInternalFormat;
	UVec3 sizes;
	uint numberOfArrayElements, numberOfFaces, numberOfMipmapLevels;
	uint bytesOfKeyValueData;
};

static ImageInfo GetImageInfoFromHeader(const KtxHeader& header)
{
	ImageInfo result = {{0,0,0}, null, ImageType_End, 0};
	if(header.sizes.y == 0 && header.sizes.z == 0) return result; //1D текстуры не поддерживаются
	if(header.numberOfFaces != 1 && header.numberOfFaces != 6) return result; //Текстуры либо кубические и содержат 6 граней, либо некубические
	result.Format = GLenumToImageFormat(ushort(header.glInternalFormat));
	if(result.Format == null) return result;
	result.MipmapCount = ushort(header.numberOfMipmapLevels);
	if(header.sizes.z == 0)
	{
		if(header.numberOfFaces == 1)
			if(header.numberOfArrayElements == 0)
				result.Type = ImageType_2D;
			else result.Type = ImageType_2DArray;
		else if(header.numberOfFaces == 6)
			if(header.numberOfArrayElements == 0)
				result.Type = ImageType_Cube;
			else result.Type = ImageType_CubeArray;
		else return {{0,0,0}, null, ImageType_End, 0};
	}
	else result.Type = ImageType_3D;
	result.Size = Max(USVec3(header.sizes), USVec3(1));
	return result;
}

ImageInfo LoaderKTX::GetInfo(IInputStream& stream) const
{
	byte headerSignature[16];
	RawReadTo(stream, headerSignature, 16);
	if(!IsValidHeader(headerSignature, 16)) return ImageInfo();
	return GetImageInfoFromHeader(RawRead<KtxHeader>(stream));
}

bool LoaderKTX::IsValidHeader(const void* header, size_t bytes) const
{
	if(bytes<12) return false;

	static const byte fileIdentifier[12] = {
		0xAB, 0x4B, 0x54, 0x58, 0x20, 0x31,
		0x31, 0xBB, 0x0D, 0x0A, 0x1A, 0x0A
	};
	return Equals(SpanOfRaw(header, bytes), fileIdentifier);
}

AnyImage LoaderKTX::Load(IInputStream& stream) const
{
	stream.PopFirstN(12); //Пропускаем идентификатор, предполагая, что он уже был проверен

	if(RawRead<uint>(stream) != 0x04030201) //Поддерживается только порядок байт файла, совпадающий с порядком байт для платформы
		return null;

	auto header = RawRead<KtxHeader>(stream);
	auto info = GetImageInfoFromHeader(header);
	if(info.Type == ImageType_End)
		return null;

	AnyImage result;
	result.LineAlignment = 4;
	result.Info = info;
	result.SwapRB = GLFormatSwapRB(ushort(header.glFormat));
	if(info.MipmapCount==0) info.MipmapCount=1;
	const size_t fullDataSize = info.CalculateFullDataSize(result.LineAlignment);
	result.Data.SetCountUninitialized(fullDataSize);
	byte* pos = result.Data.Data();

	stream.PopFirstN(header.bytesOfKeyValueData); //Пропускаем метаданные

	for(ushort i=0; i<info.MipmapCount; i++)
	{
		const uint imageSize = RawRead<uint>(stream);
		if(result.Info.Type == ImageType_Cube)
		{
			for(ushort j=0; j<6; j++)
			{
				RawReadTo(stream, pos, imageSize);
				pos += imageSize;
				pos += 3-(imageSize+3)%4;
			}
			continue;
		}
		RawReadTo(stream, pos, imageSize);
		pos += imageSize;
		pos += 3 - (imageSize + 3) % 4;
	}
	return result;
}

INTRA_WARNING_DISABLE_GLOBAL_CONSTRUCTION
const LoaderKTX LoaderKTX::Instance;
INTRA_END

#endif
