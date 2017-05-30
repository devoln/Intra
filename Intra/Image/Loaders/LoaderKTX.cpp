#ifndef INTRA_NO_KTX_LOADER

#include "Image/Loaders/LoaderKTX.h"

#include "Cpp/Warnings.h"

#include "Image/AnyImage.h"
#include "Image/Bindings/GLenumFormats.h"

namespace Intra { namespace Image {

INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS

struct KtxHeader
{
	uint glType, glTypeSize;
	uint glFormat, glInternalFormat, glBaseInternalFormat;
	Math::UVec3 sizes;
	uint numberOfArrayElements, numberOfFaces, numberOfMipmapLevels;
	uint bytesOfKeyValueData;
};

static ImageInfo GetImageInfoFromHeader(const KtxHeader& header)
{
	ImageInfo result = {{0,0,0}, null, ImageType_End, 0};
	if(header.sizes.y==0 && header.sizes.z==0) return result; //1D текстуры не поддерживаются
	if(header.numberOfFaces!=1 && header.numberOfFaces!=6) return result; //Текстуры либо кубические и содержат 6 граней, либо некубические
	result.Format = GLenumToImageFormat(ushort(header.glInternalFormat));
	if(result.Format==null) return result;
	result.MipmapCount = ushort(header.numberOfMipmapLevels);
	if(header.sizes.z==0)
	{
		if(header.numberOfFaces==1)
			if(header.numberOfArrayElements==0)
				result.Type = ImageType_2D;
			else result.Type = ImageType_2DArray;
		else if(header.numberOfFaces==6)
			if(header.numberOfArrayElements==0)
				result.Type = ImageType_Cube;
			else result.Type = ImageType_CubeArray;
		else return {{0,0,0}, null, ImageType_End, 0};
	}
	else result.Type = ImageType_3D;
	result.Size = Math::Max(Math::USVec3(header.sizes), Math::USVec3(1));
	return result;
}

ImageInfo LoaderKTX::GetInfo(InputStream stream) const
{
	byte headerSignature[16];
	stream.ReadRawTo(headerSignature, 16);
	if(!IsValidHeader(headerSignature, 16)) return ImageInfo();
	return GetImageInfoFromHeader(stream.ReadRaw<KtxHeader>());
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

AnyImage LoaderKTX::Load(InputStream stream) const
{
	stream.PopFirstN(12); //Пропускаем идентификатор, предполагая, что он уже был проверен

	if(stream.ReadRaw<uint>() != 0x04030201) //Поддерживается только порядок байт файла, совпадающий с порядком байт для платформы
		return null;

	auto header = stream.ReadRaw<KtxHeader>();
	auto info = GetImageInfoFromHeader(header);
	if(info.Type==ImageType_End)
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
		uint imageSize = stream.ReadRaw<uint>();
		if(result.Info.Type==ImageType_Cube)
		{
			for(ushort j=0; j<6; j++)
			{
				stream.ReadRawTo(pos, imageSize);
				pos += imageSize;
				pos += 3-(imageSize+3)%4;
			}
			continue;
		}
		stream.ReadRawTo(pos, imageSize);
		pos += imageSize;
		pos += 3-(imageSize+3)%4;
	}
	return result;
}

const LoaderKTX LoaderKTX::Instance;

INTRA_WARNING_POP

}}

#endif
