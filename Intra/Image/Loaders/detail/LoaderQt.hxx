#pragma once

#include "Image/Loaders/LoaderPlatform.h"
#include "Concepts/IInput.h"
#include "Math/Math.h"
#include "Cpp/Intrinsics.h"

#include "Cpp/Warnings.h"

INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS

#include <QtGui/QImage>

namespace Intra { namespace Image {

//Загрузить изображение из BMP, JPG, PNG или GIF файла
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

	QImage img;
	img.loadFromData(fileData.Data(), size);
	AnyImage result;
	result.Info.Size = {img.width(), img.height(), 1};
	const auto qtformat = img.format();
	switch(qtformat)
	{
	case QImage::Format_ARGB32:
		result.Info.Format = ImageFormat::RGBA8;
		result.SwapRB = true;
		break;
	
	case QImage::Format_RGB32:
		result.Info.Format = ImageFormat::RGBX8;
		result.SwapRB = true;
		break;

	case QImage::Format_RGB888:
		result.Info.Format = ImageFormat::RGB8;
		result.SwapRB = false;
		break;

	case QImage::Format_RGBX8888:
		result.Info.Format = ImageFormat::RGBX8;
		result.SwapRB = false;
		break;

	case QImage::Format_RGBX8888:
		result.Info.Format = ImageFormat::RGBA8;
		result.SwapRB = false;
		break;

	case QImage::Format_Indexed8: case QImage::Format_Grayscale8:
		result.Info.Format = ImageFormat::Luminance8;
		result.SwapRB = false;
		break;

	case QImage::Format_Alpha8:
		result.Info.Format = ImageFormat::Alpha8;
		result.SwapRB = false;
		break;

	default:
		return null;
	}

	result.Info.Type = ImageType_2D;
	result.Data.Clear();
	const size_t sizeInBytes = result.Info.CalculateFullDataSize(1);
	result.Data.SetCountUninitialized(sizeInBytes);
	C::memcpy(result.Data.Data(), img.bits(), sizeInBytes);
}

}}

INTRA_WARNING_POP
