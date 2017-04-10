#include "Image/Loaders/LoaderPlatform.h"
#include "Math/Math.h"
#include "Range/Polymorphic/InputRange.h"

#if(INTRA_LIBRARY_IMAGE_LOADING==INTRA_LIBRARY_IMAGE_LOADING_STB)

#error "INTRA_LIBRARY_IMAGE_LOADING_STB is not implemented!"

#elif(INTRA_LIBRARY_IMAGE_LOADING==INTRA_LIBRARY_IMAGE_LOADING_DevIL)

#include <IL/il.h>
#pragma comment(lib, "DevIL.lib")

namespace Intra { namespace Image {

//Загрузить изображение из BMP, JPG или GIF файла
AnyImage LoadWithPlatform(InputStream stream)
{
	Array<char> fileData;
	size_t capacity = 1 << 20;
	size_t size = 0;
	while(!stream.Empty())
	{
		size_t oldSize = fileData.Length();
		fileData.SetCountUninitialized(capacity);
		size = oldSize + stream.ReadRawTo(fileData(oldSize, capacity));
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
	result.Data.SetLengthUninitialized(GetSize());
	C::memcpy(result.Data.First, ilGetData(), result.GetSize());
	ilDeleteImage(handle);
}

}

#elif(INTRA_LIBRARY_IMAGE_LOADING==INTRA_LIBRARY_IMAGE_LOADING_Gdiplus)

#ifdef WIN32_LEAN_AND_MEAN
#undef WIN32_LEAN_AND_MEAN
#endif

using Intra::Math::GLSL::min;
using Intra::Math::GLSL::max;

struct IUnknown;

#ifdef _MSC_VER
#pragma warning(push, 0)
#endif

#include <olectl.h>

//Поддерживает BMP, GIF, JPEG, PNG, TIFF, Exif, WMF, и EMF. Не работает в WinRT \ Windows Phone
#include <gdiplus.h>

#ifdef _MSC_VER
#pragma warning(pop)
#pragma comment(lib, "gdiplus.lib")
#endif

namespace Intra { namespace Image {

//Загрузить изображение из BMP, JPG или GIF файла
AnyImage LoadWithPlatform(InputStream stream)
{
	if(stream.Empty()) return null;

	using namespace Gdiplus;
	using namespace Gdiplus::DllExports;

	struct InitGDIP
	{
		InitGDIP()
		{
			GdiplusStartupInput input;
			ULONG_PTR token;
			GdiplusStartup(&token, &input, 0);
		}
	};
	static InitGDIP sInitGDIP;

	//Мы не знаем размер потока, поэтому считываем его целиком в global-память, которая нужна для создания потока
	size_t size = 1 << 20;
	HGLOBAL glob = null;
	char* raw = null;
	for(;;)
	{
		HGLOBAL oldGlob = glob;
		Span<char> oldData = {raw, size/2};
		glob = GlobalAlloc(0, size);
		if(glob==null) return null;
		raw = static_cast<char*>(GlobalLock(glob));
		Span<char> range = {raw, size};
		if(oldGlob)
		{
			Algo::CopyToAdvance(oldData, range);
			GlobalUnlock(oldGlob);
			GlobalFree(oldGlob);
		}
		stream.ReadRawToAdvance(range);
		if(stream.Empty()) break;
		size *= 2;
	}
	GlobalUnlock(glob);
	
	IStream* istream;
	if(FAILED(CreateStreamOnHGlobal(glob, true, &istream)))
		return null;
	
	GpBitmap* bmp;
	GdipCreateBitmapFromStream(istream, &bmp);
	istream->Release();

	//GdipImageRotateFlip(bmp, Gdiplus::RotateNoneFlipY);

	uint width, height;
	GdipGetImageWidth(bmp, &width);
	GdipGetImageHeight(bmp, &height);
	
	AnyImage result;
	result.Info.Size = {width, height, 1};
	result.LineAlignment = 1;

	PixelFormat format;
	GdipGetImagePixelFormat(bmp, &format);
	
	if(format==PixelFormat32bppARGB)
		result.Info.Format = ImageFormat::RGBA8;
	else if(format==PixelFormat32bppRGB || format==PixelFormat24bppRGB)
	{
		result.Info.Format = ImageFormat::RGB8;
		format = PixelFormat24bppRGB;
	}
	else if(format==PixelFormat16bppARGB1555 || format==PixelFormat16bppRGB555)
	{
		result.Info.Format = ImageFormat::RGB5A1;
		format = PixelFormat16bppARGB1555;
	}
	else if(format==PixelFormat16bppRGB565)
		result.Info.Format = ImageFormat::RGB565;
	else if(format & PixelFormatIndexed)
	{
		result.Info.Format = ImageFormat::RGBA8;
		format = PixelFormat32bppARGB;
	}
	else if(format==PixelFormat16bppGrayScale)
		result.Info.Format = ImageFormat::Luminance16;
	else if(format & PixelFormatAlpha)
	{
		result.Info.Format = ImageFormat::RGBA8;
		format = PixelFormat32bppARGB;
	}
	else
	{
		result.Info.Format = ImageFormat::RGB8;
		format = PixelFormat24bppRGB;
	}

	result.SwapRB = true;
	result.Info.Type = ImageType_2D;

	result.Data.SetCountUninitialized(result.Info.CalculateMipmapDataSize(0, result.LineAlignment));
	result.Data.TrimExcessCapacity();

	BitmapData data = {
		result.Info.Size.x, result.Info.Size.y,
		result.Info.Size.x*result.Info.Format.BytesPerPixel(),
		format, result.Data.Data(), 0
	};
	GdipBitmapLockBits(bmp, null, ImageLockModeRead|ImageLockModeUserInputBuf, format, &data);
	GdipDisposeImage(bmp);
	return result;
}

}}

#elif(INTRA_LIBRARY_IMAGE_LOADING==INTRA_LIBRARY_IMAGE_LOADING_Qt)

#include <QtGui/QImage>

namespace Intra { namespace Image {

//Загрузить изображение из BMP, JPG, PNG или GIF файла
Image LoadWithPlatform(InputStream stream)
{
	Array<char> fileData;
	size_t capacity = 1 << 20;
	size_t size = 0;
	while(!stream.Empty())
	{
		size_t oldSize = fileData.Length();
		fileData.SetCountUninitialized(capacity);
		size = oldSize + stream.ReadRawTo(fileData(oldSize, capacity));
		capacity *= 2;
	}

	QImage img;
	img.loadFromData(fileData.Data(), size);
	Image result;
	result.Size = {img.width(), img.height()};
	const auto qtformat = img.format();
	switch(qtformat)
	{
	case QImage::Format_ARGB32: case QImage::Format_RGB32:
		result.Info.Format = ImageFormat::RGBA8;
		result.SwapRB = true;
		break;

	case QImage::Format_RGB888:
		result.Info.Format = ImageFormat::RGB8;
		result.SwapRB = true;
		break;

	case QImage::Format_Indexed8:
		result.Info.Format = ImageFormat::Luminance8;
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

#elif(INTRA_LIBRARY_IMAGE_LOADING==INTRA_LIBRARY_IMAGE_LOADING_SDL)


#endif
