#pragma once

#include "Image/Loaders/LoaderPlatform.h"
#include "Utils/IInput.h"
#include "Math/ShaderMath.h"

using Intra::ShaderMath::min;
using Intra::ShaderMath::max;


INTRA_PUSH_DISABLE_ALL_WARNINGS
#ifdef WIN32_LEAN_AND_MEAN
#undef WIN32_LEAN_AND_MEAN
#endif
struct IUnknown;
#include <OleCtl.h>
#include <gdiplus.h> //Supports BMP, GIF, JPEG, PNG, TIFF, Exif, WMF, и EMF. Doesn't work on WinRT \ Windows Phone

#ifdef _MSC_VER
#pragma comment(lib, "gdiplus.lib")
#endif
INTRA_WARNING_POP

INTRA_BEGIN
//Загрузить изображение из BMP, JPG или GIF файла
AnyImage LoadWithPlatform(IInputStream& stream)
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
			GdiplusStartup(&token, &input, null);
		}
	};
	static InitGDIP sInitGDIP;

	//Мы не знаем размер потока, поэтому считываем его целиком в global-память, которая нужна для создания потока GDI+
	size_t size = 1 << 20;
	HGLOBAL glob = null;
	char* raw = null;
	for(;;)
	{
		HGLOBAL oldGlob = glob;
		Span<char> oldData = {raw, size/2};
		glob = GlobalAlloc(0, size);
		if(!glob) return null;
		raw = static_cast<char*>(GlobalLock(glob));
		Span<char> range = {raw, size};
		if(oldGlob)
		{
			WriteTo(oldData, range);
			GlobalUnlock(oldGlob);
			GlobalFree(oldGlob);
		}
		RawReadTo(stream, range);
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
	
	if(format == PixelFormat32bppARGB)
		result.Info.Format = ImageFormat::RGBA8;
	else if(format == PixelFormat32bppRGB || format == PixelFormat24bppRGB)
	{
		result.Info.Format = ImageFormat::RGB8;
		format = PixelFormat24bppRGB;
	}
	else if(format == PixelFormat16bppARGB1555 || format == PixelFormat16bppRGB555)
	{
		result.Info.Format = ImageFormat::RGB5A1;
		format = PixelFormat16bppARGB1555;
	}
	else if(format == PixelFormat16bppRGB565)
		result.Info.Format = ImageFormat::RGB565;
	else if(format & PixelFormatIndexed)
	{
		result.Info.Format = ImageFormat::RGBA8;
		format = PixelFormat32bppARGB;
	}
	else if(format == PixelFormat16bppGrayScale)
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
INTRA_END
