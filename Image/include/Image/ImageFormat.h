#pragma once

#include "Platform/CppWarnings.h"
#include "Platform/FundamentalTypes.h"
#include "Math/Vector.h"
#include "Data/ValueType.h"

namespace Intra { namespace Image {

INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS

//Все компоненты в форматах указаны в порядке возрастания значимости бит в порядке увеличения байта
struct ImageFormat
{
	enum I: byte
	{
		None = 0,

		//Нормализованные форматы
		// - Беззнаковые
		Red8, Luminance8, Alpha8, RG8, LuminanceAlpha8, RGB8, RGBA8, RGBX8,  //8 бит на канал
		Red16, Luminance16, Alpha16, RG16, LuminanceAlpha16, RGB16, RGBA16, RGBX16, //16 бит на канал
		Red32, Luminance32, Alpha32, RG32, LuminanceAlpha32, RGB32, RGBA32, RGBX32, //32 бита на канал
		BGR233, RGB565, A1_BGR5, X1_BGR5, ABGR4, XBGR4, RGB10A2, RGB10, //Упакованные
		RGB5, RGB5A1, RGBA4, RGBX4,

		// - Знаковые
		Red8s, RG8s, RGB8s, RGBA8s,  //8 бит на канал
		Red16s, RG16s, RGB16s, RGBA16s, //16 бит на канал
		Red32s, RG32s, RGB32s, RGBA32s, //32 бита на канал

		// - sRGB
		sRGB8, sRGB8_A8,

		Red16f, Luminance16f, RG16f, RGB16f, RGBA16f, //Половинная точность
		Red32f, Luminance32f, RG32f, RGB32f, RGBA32f, //Одинарная точность
		R11G11B10f, //Упакованные форматы
		RGB9E5, RGBE8, //Общая экспонента у всех компонентов


		//Беззнаковые
		Red8u, RG8u, RGB8u, RGBA8u,  //8 бит на канал
		Red16u, RG16u, RGB16u, RGBA16u, //16 бит на канал
		Red32u, RG32u, RGB32u, RGBA32u, //32 бита на канал
		RGB10A2u,                       //Упакованный

		//Знаковые
		Red8i, RG8i, RGB8i, RGBA8i,  //8 бит на канал
		Red16i, RG16i, RGB16i, RGBA16i, //16 бит на канал
		Red32i, RG32i, RGB32i, RGBA32i, //32 бита на канал

		//Форматы для глубины и трафарета
		Depth16, Depth24, Depth24Stencil8, Depth32, Depth32f, Depth32fStencil8,



		//Сжатые форматы
		DXT1_RGB = 100, DXT1_RGBA, DXT3_RGBA, DXT5_RGBA, DXT1_sRGB, DXT1_sRGB_A, DXT3_sRGB_A, DXT5_sRGB_A,
		ETC1_RGB, EAC_Red, EAC_RG, EAC_Rs, EAC_RGs, ETC2_RGB, ETC2_sRGB, ETC2_RGBA, ETC2_RGB_BinAlpha, ETC2_sRGB_BinAlpha,
		ATC_RGB, ATC_RGB_ExplicitAlpha, ATC_RGB_InterpolatedAlpha,
		BPTC_RGBA, BPTC_sRGB_A, BPTC_RGBf, BPTC_RGBuf,
		RGTC_Red, RGTC_SignedRed, RGTC_RG, RGTC_SignedRG,
		LATC_Luminance, LATC_SignedLuminance, LATC_LuminanceAlpha, LATC_SignedLuminanceAlpha,
		PVRTC_RGB_4bpp, PVRTC_RGB_2bpp, PVRTC_RGBA_4bpp, PVRTC_RGBA_2bpp,

		//Базовые форматы
		R = 200, Luminance, Alpha, LuminanceAlpha, RG, RGB, RGBA,  //Нормализованные беззнаковые
		Rs, RGs, RGBs, RGBAs, //Нормализованные знаковые
		Ru, RGu, RGBu, RGBAu, //Целые беззнаковые
		Ri, RGi, RGBi, RGBAi, //Целые знаковые
		Rf, Luminancef, RGf, RGBf, RGBAf, //С плавающей запятой
		sRGB, sRGB_A, //sRGB
		Depth, DepthF, DepthStencil, DepthF_Stencil,

		FirstOfNormalized = Red8, FirstOfUncompressed = FirstOfNormalized, EndOfNormalized = sRGB8_A8+1,
		FirstOfFloatingPoint = Red16f, EndOfFloatingPoint = RGBE8+1,
		FirstOfUnsignedIntegral = Red8u, FirstOfIntegral = FirstOfUnsignedIntegral, EndOfUnsignedIntegral = RGB10A2u+1,
		FirstOfSignedIntegral = Red8i, EndOfSignedIntegral = RGBA32i+1, EndOfIntegral = EndOfSignedIntegral,
		FirstOfDepth = Depth16, EndOfDepth = Depth32fStencil8+1, EndOfUncompressed = EndOfDepth,

		FirstOfCompressed = DXT1_RGB, EndOfCompressed = PVRTC_RGBA_2bpp+1, FirstOfBasic = R,
		EndOfBasic = DepthF_Stencil+1, End = EndOfBasic
	};

	static_assert(FirstOfCompressed>=EndOfUncompressed && FirstOfBasic>=EndOfCompressed, "ImageFormat enum overlaps itself!");

	I value;

	explicit ImageFormat(ushort val): value(I(val)) {}
	ImageFormat(I vt): value(vt) {}
	ImageFormat(null_t = null): value(None) {}
	ImageFormat(const ImageFormat& rhs) = default;

	ImageFormat& operator=(null_t) { value = None; return *this; }
	ImageFormat& operator=(const ImageFormat& rhs) = default;

	bool operator==(null_t) const { return value==None; }
	bool operator!=(null_t) const { return !operator==(null); }
	bool operator==(byte rhs) const { return value==rhs; }
	bool operator!=(byte rhs) const { return !operator==(rhs); }
	bool operator==(ImageFormat rhs) const { return value==rhs.value; }
	bool operator!=(ImageFormat rhs) const { return !operator==(rhs); }

	enum class FormatClass: byte { Normalized, FloatingPoint, Integral, Depth, Compressed, Base, Invalid };

	byte BitsPerPixel() const;
	byte BitsPerComponent() const;
	byte BytesPerPixel() const { return byte(!IsCompressed()? BitsPerPixel()/8: 0); }

	bool IsValid() const;

	bool IsNormalized() const;
	bool IsCompressed() const;
	bool IsCompressedBC1_BC7() const;
	bool IsFloatingPoint() const;
	bool IsIntegral() const;
	bool IsSigned() const;
	bool IsBasic() const;
	bool HasDepth() const;
	bool HasStencil() const;
	bool IsLuminance() const;
	bool HasLuminance() const;
	bool IsAlpha() const;
	bool HasAlpha() const;
	bool HasColor() const; //Возвращает, является ли текстура цветовой, то есть имеет как минимум красный канал
	bool IsSRGB() const;

	//Упакованным считается несжатый формат, число бит на компоненту которого не кратно 8
	bool IsPacked() const;
	Math::UVec4 GetBitMasks(bool swapRB = false) const;

	byte ComponentCount() const;
	ValueType GetComponentType() const;
	ValueType GetValueType() const;
	ImageFormat GetBasicFormat() const;
	ImageFormat ToSRGB() const;
	ImageFormat ToNonSRGB() const;
	StringView ToString() const;
	static ImageFormat FromString(StringView str, bool* exists = null);
};


INTRA_WARNING_POP

}
using Image::ImageFormat;
}
