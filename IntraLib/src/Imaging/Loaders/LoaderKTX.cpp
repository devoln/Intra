#include "Imaging/Image.h"
#include "Imaging/Bindings/GLenumFormats.h"

namespace Intra {

using namespace Math;
using namespace IO;

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
	if(header.sizes.y==0 && header.sizes.z==0) return result; //1D текстуры не поддерживаются
	if(header.numberOfFaces!=1 && header.numberOfFaces!=6) return result; //Текстуры либо кубические и содержат 6 граней, либо некубические
	result.Format = GLenumToImageFormat((ushort)header.glInternalFormat);
	if(result.Format==null) return result;
	result.MipmapCount=(ushort)header.numberOfMipmapLevels;
	if(header.sizes.z==0)
	{
		if(header.numberOfFaces==1)
			if(header.numberOfArrayElements==0) result.Type=ImageType_2D;
			else result.Type=ImageType_2DArray;
		else if(header.numberOfFaces==6)
			if(header.numberOfArrayElements==0) result.Type=ImageType_Cube;
			else result.Type=ImageType_CubeArray;
		else return {{0,0,0}, null, ImageType_End, 0};
	}
	else result.Type = ImageType_3D;
	result.Size = Max((USVec3)header.sizes, USVec3(1));
	return result;
}

ImageInfo pe_get_ktx_info(byte header[64])
{
	//if(memcmp(header, "DDS ", 4)!=0) return errResult;
	return GetImageInfoFromHeader(*(KtxHeader*)(header+16));
}

#ifndef NO_KTX_LOADER
void Image::loadKTX(IInputStream* s, uint bytes)
{
	auto startPos=s->GetPos();

	s->Skip(12); //Пропускаем идентификатор, предполагая, что он уже был проверен

	if(s->Read<uint>() != 0x04030201) //Поддерживается только порядок байт файла, совпадающий с порядком байт для платформы
	{
		s->SetPos(startPos+bytes);
		return;
	}

	auto header=s->Read<KtxHeader>();
	auto info=GetImageInfoFromHeader(header);
	if(info.Type==ImageType_End)
	{
		s->SetPos(startPos+bytes);
		return;
	}

	LineAlignment = 4;
	Info = info;
	SwapRB = GLFormatSwapRB((ushort)header.glFormat);
	if(info.MipmapCount==0) info.MipmapCount=1;
	const size_t fullDataSize = info.CalculateFullDataSize(LineAlignment);
	Data.Clear();
	Data.SetCountUninitialized(fullDataSize);
	byte* pos = Data.Data();

	s->Skip(header.bytesOfKeyValueData); //Пропускаем метаданные

	for(ushort i=0; i<info.MipmapCount; i++)
	{
		uint imageSize = s->Read<uint>();
		if(Info.Type==ImageType_Cube)
		{
			for(ushort j=0; j<6; j++)
			{
				s->ReadData(pos, imageSize);
				pos += imageSize;
				pos += 3-(imageSize+3)%4;
			}
			continue;
		}
		s->ReadData(pos, imageSize);
		pos += imageSize;
		pos += 3-(imageSize+3)%4;
	}
}
#endif

}
