#include "Image/Bindings/GLenumFormats.h"
#include "Image/ImageFormat.h"
#include "Container/Associative/HashMap.h"
#include "Image/ImageInfo.h"

namespace Intra { namespace Image {

ushort ImageFormatToGLInternal(ImageFormat format, bool useSwizzling)
{
	if(!format.IsValid()) return 0;
	static const ushort uncompressedTable[] = {0,
		GL::R8, GL::LUMINANCE8, GL::ALPHA8, GL::RG8, GL::LUMINANCE8_ALPHA8, GL::RGB8, GL::RGBA8, GL::RGBA8,
		GL::R16, GL::LUMINANCE16, GL::ALPHA16, GL::RG16, GL::LUMINANCE16_ALPHA16, GL::RGB16, GL::RGBA16, GL::RGBA16,
		0,0,0,0,0,0,0,0,
		GL::R3_G3_B2, GL::RGB565, GL::RGB5_A1, GL::RGB5_A1, GL::RGBA4, GL::RGBA4, GL::RGB10_A2, GL::RGB10_A2,
		0,0,0,0,

		GL::R8_SNORM, GL::RG8_SNORM, GL::RGB8_SNORM, GL::RGBA8_SNORM,
		GL::R16_SNORM, GL::RG16_SNORM, GL::RGB16_SNORM, GL::RGBA16_SNORM,
		0,0,0,0,

		GL::SRGB8, GL::SRGB8_ALPHA8,

		GL::R16F, GL::LUMINANCE16F, GL::RG16F, GL::RGB16F, GL::RGBA16F,
		GL::R32F, GL::LUMINANCE32F, GL::RG32F, GL::RGB32F, GL::RGBA32F,
		GL::R11F_G11F_B10F,
		GL::RGB9_E5, 0,

		GL::R8UI, GL::RG8UI, GL::RGB8UI, GL::RGBA8UI,
		GL::R16UI, GL::RG16UI, GL::RGB16UI, GL::RGBA16UI,
		GL::R32UI, GL::RG32UI, GL::RGB32UI, GL::RGBA32UI,
		GL::RGB10_A2UI,

		GL::R8I, GL::RG8I, GL::RGB8I, GL::RGBA8I,
		GL::R16I, GL::RG16I, GL::RGB16I, GL::RGBA16I,
		GL::R32I, GL::RG32I, GL::RGB32I, GL::RGBA32I,

		GL::DEPTH_COMPONENT16, GL::DEPTH_COMPONENT24, GL::DEPTH24_STENCIL8, GL::DEPTH_COMPONENT32, GL::DEPTH_COMPONENT32F, GL::DEPTH32F_STENCIL8
	};
	INTRA_CHECK_TABLE_SIZE(uncompressedTable, ImageFormat::EndOfUncompressed);

	static const ushort compressedTable[]=
	{
		//DXT
		GL::COMPRESSED_RGB_S3TC_DXT1_EXT, GL::COMPRESSED_RGBA_S3TC_DXT1_EXT, GL::COMPRESSED_RGBA_S3TC_DXT3_EXT, GL::COMPRESSED_RGBA_S3TC_DXT5_EXT,
		GL::COMPRESSED_SRGB_S3TC_DXT1_EXT, GL::COMPRESSED_SRGB_ALPHA_S3TC_DXT1_EXT, GL::COMPRESSED_SRGB_ALPHA_S3TC_DXT3_EXT, GL::COMPRESSED_SRGB_ALPHA_S3TC_DXT5_EXT,

		//ETC
		/*GL::ETC1_RGB8_OES*/ GL::COMPRESSED_RGB8_ETC2, GL::COMPRESSED_R11_EAC, GL::COMPRESSED_RG11_EAC,
		GL::COMPRESSED_SIGNED_R11_EAC, GL::COMPRESSED_SIGNED_RG11_EAC, GL::COMPRESSED_RGB8_ETC2, GL::COMPRESSED_SRGB8_ETC2,
		GL::COMPRESSED_RGBA8_ETC2_EAC, GL::COMPRESSED_RGB8_PUNCHTHROUGH_ALPHA1_ETC2, GL::COMPRESSED_SRGB8_PUNCHTHROUGH_ALPHA1_ETC2,

		//ATC
		GL::ATC_RGB_AMD, GL::ATC_RGBA_EXPLICIT_ALPHA_AMD, GL::ATC_RGBA_INTERPOLATED_ALPHA_AMD,

		//BPTC
		GL::COMPRESSED_RGBA_BPTC_UNORM, GL::COMPRESSED_SRGB_ALPHA_BPTC_UNORM, GL::COMPRESSED_RGB_BPTC_SIGNED_FLOAT, GL::COMPRESSED_RGB_BPTC_UNSIGNED_FLOAT,

		//RGTC
		GL::COMPRESSED_RED_RGTC1, GL::COMPRESSED_SIGNED_RED_RGTC1, GL::COMPRESSED_RG_RGTC2, GL::COMPRESSED_SIGNED_RG_RGTC2,

		//LATC
		GL::COMPRESSED_LUMINANCE_LATC1_EXT, GL::COMPRESSED_SIGNED_LUMINANCE_LATC1_EXT,
		GL::COMPRESSED_LUMINANCE_ALPHA_LATC2_EXT, GL::COMPRESSED_SIGNED_LUMINANCE_ALPHA_LATC2_EXT,

		//PVRTC
		GL::COMPRESSED_RGB_PVRTC_4BPPV1_IMG, GL::COMPRESSED_RGB_PVRTC_2BPPV1_IMG, GL::COMPRESSED_RGBA_PVRTC_4BPPV1_IMG, GL::COMPRESSED_RGBA_PVRTC_2BPPV1_IMG,
	};
	INTRA_CHECK_TABLE_SIZE(compressedTable, ImageFormat::EndOfCompressed-ImageFormat::FirstOfCompressed);

	static const ushort basicFormatTable[]= //Пользователю неважно, какой именно будет формат из соответствующих указанному базовому, поэтому реализация выбирает автоматически
	{
		GL::RED, GL::LUMINANCE, GL::ALPHA, GL::LUMINANCE_ALPHA, GL::RG, GL::RGB, GL::RGBA,
		GL::R8_SNORM, GL::RG8_SNORM, GL::RGB8_SNORM, GL::RGBA8_SNORM,
		GL::R8UI, GL::RG8UI, GL::RGB8UI, GL::RGBA8UI,
		GL::R8I, GL::RG8I, GL::RGB8I, GL::RGBA8I,
		GL::R16F, GL::LUMINANCE16F, GL::RG16F, GL::RGB16F, GL::RGBA16F,
		GL::SRGB8, GL::SRGB8_ALPHA8,
		GL::DEPTH_COMPONENT, GL::DEPTH_COMPONENT, GL::DEPTH_STENCIL, GL::DEPTH_STENCIL
	};
	INTRA_CHECK_TABLE_SIZE(basicFormatTable, ImageFormat::EndOfBasic-ImageFormat::FirstOfBasic);

	ushort result;
	if(format.value<ImageFormat::EndOfUncompressed)
		result = uncompressedTable[format.value];
	else if(format.value<ImageFormat::EndOfCompressed)
	{
		result = compressedTable[format.value-ImageFormat::FirstOfCompressed];
	}
	else if(format.value<ImageFormat::EndOfBasic)
		result = basicFormatTable[format.value-ImageFormat::FirstOfBasic];
	else return 0;

	if(useSwizzling && (format.HasLuminance() || format.IsAlpha()))
		switch(result)
	{
	case GL::LUMINANCE: case GL::ALPHA:
		return GL::RED;

	case GL::LUMINANCE8: case GL::ALPHA8:
		return GL::R8;

	case GL::LUMINANCE_ALPHA:
		return GL::RG;

	case GL::LUMINANCE8_ALPHA8:
		return GL::RG8;

	case GL::LUMINANCE16_ALPHA16:
		return GL::RG16;

	case GL::LUMINANCE16: case GL::ALPHA16:
		return GL::R16;

	case GL::LUMINANCE16F:
		return GL::R16F;

	case GL::LUMINANCE32F:
		return GL::R32F;

	case GL::COMPRESSED_LUMINANCE_LATC1_EXT:
		return GL::COMPRESSED_RED_RGTC1;

	case GL::COMPRESSED_SIGNED_LUMINANCE_LATC1_EXT:
		return GL::COMPRESSED_SIGNED_RED_RGTC1;

	case GL::COMPRESSED_LUMINANCE_ALPHA_LATC2_EXT:
		return GL::COMPRESSED_RG_RGTC2;

	case GL::COMPRESSED_SIGNED_LUMINANCE_ALPHA_LATC2_EXT:
		return GL::COMPRESSED_SIGNED_RG_RGTC2;

	default:;
	}
	return result;
}

ushort ImageFormatToGLExternal(ImageFormat format, bool swapRB, bool useSwizzling)
{
	if(!format.IsValid() || format.IsCompressed()) return 0;
	static const ushort uncompressedTable[]=
	{0,
		GL::RED, GL::LUMINANCE, GL::ALPHA, GL::RG, GL::LUMINANCE_ALPHA, GL::RGB, GL::RGBA, GL::RGBA,
		GL::RED, GL::LUMINANCE, GL::ALPHA, GL::RG, GL::LUMINANCE_ALPHA, GL::RGB, GL::RGBA, GL::RGBA,
		GL::RED, GL::LUMINANCE, GL::ALPHA, GL::RG, GL::LUMINANCE_ALPHA, GL::RGB, GL::RGBA, GL::RGBA,
		GL::RGB, GL::RGB, GL::RGBA, GL::RGBA, GL::RGBA, GL::RGBA, GL::RGBA, GL::RGBA,
		0,0,0,0,

		GL::RED, GL::RG, GL::RGB, GL::RGBA,
		GL::RED, GL::RG, GL::RGB, GL::RGBA,
		GL::RED, GL::RG, GL::RGB, GL::RGBA,

		GL::RGB, GL::RGBA,

		GL::RED, GL::LUMINANCE, GL::RG, GL::RGB, GL::RGBA,
		GL::RED, GL::LUMINANCE, GL::RG, GL::RGB, GL::RGBA,
		GL::RGB, GL::RGB, 0,

		GL::RED_INTEGER, GL::RG_INTEGER, GL::RGB_INTEGER, GL::RGBA_INTEGER,
		GL::RED_INTEGER, GL::RG_INTEGER, GL::RGB_INTEGER, GL::RGBA_INTEGER,
		GL::RED_INTEGER, GL::RG_INTEGER, GL::RGB_INTEGER, GL::RGBA_INTEGER,
		GL::RGBA_INTEGER,

		GL::RED_INTEGER, GL::RG_INTEGER, GL::RGB_INTEGER, GL::RGBA_INTEGER,
		GL::RED_INTEGER, GL::RG_INTEGER, GL::RGB_INTEGER, GL::RGBA_INTEGER,
		GL::RED_INTEGER, GL::RG_INTEGER, GL::RGB_INTEGER, GL::RGBA_INTEGER,

		GL::DEPTH_COMPONENT, GL::DEPTH_COMPONENT, GL::DEPTH_STENCIL, GL::DEPTH_COMPONENT, GL::DEPTH_COMPONENT, GL::DEPTH_STENCIL
	};
	INTRA_CHECK_TABLE_SIZE(uncompressedTable, ImageFormat::EndOfUncompressed);

	static const ushort basicFormatTable[]= //Пользователю неважно, какой именно будет формат из соответствующих указанному базовому, поэтому реализация выбирает автоматически
	{
		GL::RED, GL::LUMINANCE, GL::ALPHA, GL::LUMINANCE_ALPHA, GL::RG, GL::RGB, GL::RGBA,
		GL::RED, GL::RG, GL::RGB, GL::RGBA,
		GL::RED_INTEGER, GL::RG_INTEGER, GL::RGB_INTEGER, GL::RGBA_INTEGER,
		GL::RED_INTEGER, GL::RG_INTEGER, GL::RGB_INTEGER, GL::RGBA_INTEGER,
		GL::RED, GL::LUMINANCE, GL::RG, GL::RGB, GL::RGBA,
		GL::RGB, GL::RGBA,
		GL::DEPTH_COMPONENT, GL::DEPTH_COMPONENT, GL::DEPTH_STENCIL, GL::DEPTH_STENCIL
	};

	bool basic = format.IsBasic();
	ushort result = basic? basicFormatTable[format.value-ImageFormat::FirstOfBasic]: uncompressedTable[format.value];
	if(useSwizzling)
	{
		if(result==GL::LUMINANCE || result==GL::ALPHA) return GL::RED;
		if(result==GL::LUMINANCE_ALPHA) return GL::RG;
	}
	if(swapRB && format.ComponentCount()>=3)
	{
		if(result==GL::RGB) return GL::BGR;
		if(result==GL::RGBA) return GL::BGRA;
		if(result==GL::RGB_INTEGER) return GL::BGR_INTEGER;
		if(result==GL::RGBA_INTEGER) return GL::BGRA_INTEGER;
	}
	return result;
}

ushort ImageFormatToGLType(ImageFormat format)
{
	if(!format.IsValid()) return 0;

	static const ushort uncompressedFormatTable[]={
		0,
		GL::UNSIGNED_BYTE, GL::UNSIGNED_BYTE, GL::UNSIGNED_BYTE, GL::UNSIGNED_BYTE,
		GL::UNSIGNED_BYTE, GL::UNSIGNED_BYTE, GL::UNSIGNED_BYTE, GL::UNSIGNED_BYTE,

		GL::UNSIGNED_SHORT, GL::UNSIGNED_SHORT, GL::UNSIGNED_SHORT, GL::UNSIGNED_SHORT,
		GL::UNSIGNED_SHORT, GL::UNSIGNED_SHORT, GL::UNSIGNED_SHORT, GL::UNSIGNED_SHORT,

		GL::UNSIGNED_INT, GL::UNSIGNED_INT, GL::UNSIGNED_INT, GL::UNSIGNED_INT,
		GL::UNSIGNED_INT, GL::UNSIGNED_INT, GL::UNSIGNED_INT, GL::UNSIGNED_INT,

		GL::UNSIGNED_BYTE_3_3_2, GL::UNSIGNED_SHORT_5_6_5,
		GL::UNSIGNED_SHORT_5_5_5_1, GL::UNSIGNED_SHORT_5_5_5_1,
		GL::UNSIGNED_SHORT_4_4_4_4, GL::UNSIGNED_SHORT_4_4_4_4,
		GL::UNSIGNED_INT_2_10_10_10_REV, GL::UNSIGNED_INT_2_10_10_10_REV,
		0,0,0,0,

		GL::BYTE, GL::BYTE, GL::BYTE, GL::BYTE,
		GL::SHORT, GL::SHORT, GL::SHORT, GL::SHORT,
		GL::INT, GL::INT, GL::INT, GL::INT,

		GL::UNSIGNED_BYTE, GL::UNSIGNED_BYTE,

		GL::HALF_FLOAT, GL::HALF_FLOAT, GL::HALF_FLOAT, GL::HALF_FLOAT, GL::HALF_FLOAT,
		GL::FLOAT, GL::FLOAT, GL::FLOAT, GL::FLOAT, GL::FLOAT,
		GL::UNSIGNED_INT_10F_11F_11F_REV,
		GL::UNSIGNED_INT_5_9_9_9_REV, 0,

		GL::UNSIGNED_BYTE, GL::UNSIGNED_BYTE, GL::UNSIGNED_BYTE, GL::UNSIGNED_BYTE,
		GL::UNSIGNED_SHORT, GL::UNSIGNED_SHORT, GL::UNSIGNED_SHORT, GL::UNSIGNED_SHORT,
		GL::UNSIGNED_INT, GL::UNSIGNED_INT, GL::UNSIGNED_INT, GL::UNSIGNED_INT,
		GL::UNSIGNED_INT_10_10_10_2,

		GL::BYTE, GL::BYTE, GL::BYTE, GL::BYTE,
		GL::SHORT, GL::SHORT, GL::SHORT, GL::SHORT,
		GL::INT, GL::INT, GL::INT, GL::INT,

		GL::UNSIGNED_SHORT, GL::UNSIGNED_INT, GL::UNSIGNED_INT, GL::UNSIGNED_INT, GL::FLOAT, GL::FLOAT_32_UNSIGNED_INT_24_8_REV
	};
	INTRA_CHECK_TABLE_SIZE(uncompressedFormatTable, ImageFormat::EndOfUncompressed);

	if(format.IsCompressed()) return 0;

	static const ushort basicFormatTable[]={
		GL::UNSIGNED_BYTE, GL::UNSIGNED_BYTE, GL::UNSIGNED_BYTE, GL::UNSIGNED_BYTE, GL::UNSIGNED_BYTE, GL::UNSIGNED_BYTE, GL::UNSIGNED_BYTE,
		GL::BYTE, GL::BYTE, GL::BYTE, GL::BYTE,
		GL::UNSIGNED_BYTE, GL::UNSIGNED_BYTE, GL::UNSIGNED_BYTE, GL::UNSIGNED_BYTE,
		GL::BYTE, GL::BYTE, GL::BYTE, GL::BYTE,
		GL::HALF_FLOAT, GL::HALF_FLOAT, GL::HALF_FLOAT, GL::HALF_FLOAT, GL::HALF_FLOAT,
		GL::UNSIGNED_BYTE, GL::UNSIGNED_BYTE,
		GL::UNSIGNED_INT, GL::FLOAT, GL::UNSIGNED_INT, GL::FLOAT_32_UNSIGNED_INT_24_8_REV
	};
	INTRA_CHECK_TABLE_SIZE(basicFormatTable, ImageFormat::EndOfBasic-ImageFormat::FirstOfBasic);

	if(format.IsBasic()) return basicFormatTable[format.value-ImageFormat::FirstOfBasic];
	return uncompressedFormatTable[format.value];
}

bool GLFormatSwapRB(ushort extFormat)
{
	return (extFormat==GL::BGR || extFormat==GL::BGRA || extFormat==GL::BGR_INTEGER || extFormat==GL::BGRA_INTEGER);
}

ImageFormat GLenumToImageFormat(ushort internalFormat)
{
	if(internalFormat==GL::ETC1_RGB8_OES) return ImageFormat::ETC1_RGB;
	static HashMap<ushort, ImageFormat> imageFormatFromGLInternal;
	if(imageFormatFromGLInternal==null)
		for(ushort i=1; i<ImageFormat::End; i++)
		{
			const auto t = ImageFormatToGLInternal(ImageFormat(i), false);
			if(t!=0) imageFormatFromGLInternal[t]=ImageFormat(i);
		}
	auto found = imageFormatFromGLInternal.Find(internalFormat);
	if(found.Empty()) return null;
	return found.First().Value;
}

ushort ImageTypeToGLTarget(ImageType type)
{
	static const ushort imageTypeConvertTable[]={
		GL::TEXTURE_1D, GL::TEXTURE_1D_ARRAY, GL::TEXTURE_2D, GL::TEXTURE_2D_ARRAY,
		GL::TEXTURE_3D, GL::TEXTURE_CUBE_MAP, GL::TEXTURE_CUBE_MAP_ARRAY
	};
	INTRA_CHECK_TABLE_SIZE(imageTypeConvertTable, ImageType_End);
	return imageTypeConvertTable[type];
}

ushort CubeFaceToGLTarget(CubeFace cf) {return ushort(GL::TEXTURE_CUBE_MAP_POSITIVE_X+byte(cf));}

ImageType GLTargetToImageType(ushort gl_Target)
{
	for(ushort i=0; i<ImageType_End; i++)
		if(ImageTypeToGLTarget(ImageType(i))==gl_Target) return ImageType(i);
	return ImageType_End;
}

}}
