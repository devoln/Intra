#pragma once

#include "Image/Loaders/LoaderPlatform.h"
#include "Utils/IInput.h"

INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS
#include <IL/il.h>
#pragma comment(lib, "DevIL.lib")
INTRA_WARNING_POP

INTRA_BEGIN
//Загрузить изображение из BMP, JPG или GIF файла
AnyImage LoadWithPlatform(IInputStream& stream)
{
	Array<char> fileData;
	size_t capacity = 1 << 20;
	size_t size = 0;
	while(!stream.Empty())
	{
		size_t oldSize = fileData.Length();
		fileData.SetCountUninitialized(capacity);
		size = oldSize + RawReadTo(stream, fileData(oldSize, capacity));
		capacity *= 2;
	}

	ilInit();
	auto handle = ilGenImage();
	ilBindImage(handle);
	ilLoadL(IL_TYPE_UNKNOWN, fileData.Data(), size);
	AnyImage result;
	result.Info.Size = {ilGetInteger(IL_IMAGE_WIDTH), ilGetInteger(IL_IMAGE_HEIGHT), 1};
	result.SwapRB = false;
	const ushort glformat = ushort(ilGetInteger(IL_IMAGE_FORMAT));
	switch(glformat)
	{
	case IL_LUMINANCE: result.Info.Format = ImageFormat::Luminance8; break;
	case IL_BGR: result.SwapRB = true;
	case IL_RGB: result.Info.Format = ImageFormat::RGB8; break;
	case IL_BGRA: result.SwapRB = true;
	case IL_RGBA: result.Info.Format = ImageFormat::RGBA8; break;
	}
	result.Info.Type = ImageType_2D;
	result.Data.Clear(); //Оптимизация: чтобы при растягивании буфера не копировалось старое содержимое, которое нам не нужно
	result.Data.SetCountUninitialized(result.Info.CalculateMipmapDataSize(0, 1));
	Misc::CopyBits(result.Data.First, ilGetData(), result.Data.Length());
	ilDeleteImage(handle);
}
INTRA_END
