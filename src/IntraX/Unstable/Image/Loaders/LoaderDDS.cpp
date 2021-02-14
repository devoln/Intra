#ifndef INTRA_NO_DDS_LOADER

#include "IntraX/Unstable/Image/Loaders/LoaderDDS.h"
#include "IntraX/Unstable/Image/AnyImage.h"
#include "IntraX/Unstable/Image/Bindings/DXGI_Formats.h"

#include "IntraX/Utils/Endianess.h"

#include "Intra/Range/Stream/RawRead.h"
#include "Intra/Range/Stream/RawWrite.h"

#include "IntraX/Container/Sequential/String.h"

namespace Intra { INTRA_BEGIN
struct DDS_PIXELFORMAT
{
	uint32LE size;
	uint32LE flags;
	char fourCC[4];
	uint32LE RGBBitCount;
	Vector4<uint32LE> rgbaBitMasks;
};

struct DDS_HEADER
{
	uint32LE size;
	uint32LE flags;
	uint32LE height, width;
	uint32LE pitchOrLinearSize;
	uint32LE depth;
	uint32LE mipMapCount;
	unsigned reserved1[11];
	DDS_PIXELFORMAT ddspf;
	uint32LE caps, caps2, caps3, caps4;
	uint32LE reserved2;
};

enum {
	DDSD_CAPS = 0x00000001,  DDSD_PIXELFORMAT = 0x00001000,
	DDSD_WIDTH = 0x00000004, DDSD_HEIGHT = 0x00000002,
	DDSD_PITCH = 0x00000008, DDSD_MIPMAPCOUNT = 0x00020000,
	DDSD_LINEARSIZE = 0x00080000, DDSD_DEPTH = 0x00800000
};

enum {
	DDSCAPS_COMPLEX = 0x00000008, DDSCAPS_TEXTURE = 0x00001000, DDSCAPS_MIPMAP = 0x00400000,
	DDSCAPS2_VOLUME = 0x00200000, DDSCAPS2_CUBEMAP = 0x00000200
};

enum {
	DDSCAPS2_CUBEMAP_POSITIVEX = 0x00000400, DDSCAPS2_CUBEMAP_NEGATIVEX = 0x00000800,
	DDSCAPS2_CUBEMAP_POSITIVEY = 0x00001000, DDSCAPS2_CUBEMAP_NEGATIVEY = 0x00002000,
	DDSCAPS2_CUBEMAP_POSITIVEZ = 0x00004000, DDSCAPS2_CUBEMAP_NEGATIVEZ = 0x00008000,
	DDSCAPS2_CUBEMAP_ALL_FACES = 0x0000FC00
};



enum D3D10_RESOURCE_DIMENSION: byte
{ 
  D3D10_RESOURCE_DIMENSION_UNKNOWN,
  D3D10_RESOURCE_DIMENSION_BUFFER,
  D3D10_RESOURCE_DIMENSION_TEXTURE1D,
  D3D10_RESOURCE_DIMENSION_TEXTURE2D,
  D3D10_RESOURCE_DIMENSION_TEXTURE3D 
};

struct DDS_HEADER_DXT10
{
	DxgiFormat dxgiFormat;
	byte unused1[3];

	D3D10_RESOURCE_DIMENSION resourceDimension;
	byte unused2[3];

	uint32LE miscFlag;
	uint32LE arraySize;
	uint32LE miscFlags2;
};


enum: unsigned {
	DDS_RESOURCE_MISC_TEXTURECUBE = 4
};

enum: unsigned {
	DDPF_ALPHAPIXELS = 1, DDPF_ALPHA = 2,
	DDPF_FOURCC = 4, DDPF_RGB = 0x40, DDPF_LUMINANCE = 0x20000
};

inline constexpr uint32 to_fourcc(const char* c)
{
	return uint32(c[0])|(uint32(c[1]) << 8u)|(uint32(c[2]) << 16u)|(uint32(c[3]) << 24u);
}

inline void from_fourcc(char* dst, unsigned f)
{
	dst[0] = char(f & 255);
	dst[1] = char((f >> 8) & 255);
	dst[2] = char((f >> 16) & 255);
	dst[3] = char((f >> 24) & 255);
}

static constexpr struct {unsigned fourcc; ImageFormat::I format;} FourCC_ImageFormat[]={
	{to_fourcc("DXT1"), ImageFormat::DXT1_RGBA},
	{to_fourcc("DXT3"), ImageFormat::DXT3_RGBA},
	{to_fourcc("DXT5"), ImageFormat::DXT5_RGBA},
	{to_fourcc("ATI1"), ImageFormat::LATC_Luminance},
	{to_fourcc("ATI2"), ImageFormat::LATC_LuminanceAlpha},
	{to_fourcc("BC4U"), ImageFormat::RGTC_Red},
	{to_fourcc("BC4S"), ImageFormat::RGTC_SignedRed},
	{to_fourcc("BC5U"), ImageFormat::RGTC_RG},
	{to_fourcc("BC5S"), ImageFormat::RGTC_SignedRG}
};

struct FormatDescriptor
{
	ImageFormat::I format;
	bool swapRB;
	UVec4 masks;
};

static const FormatDescriptor D3d9Formats[] =
{
	{ImageFormat::RGB8, true, {0xFF0000, 0xFF00, 0xFF, 0}},
	{ImageFormat::RGBA8, true, {0xFF0000, 0xFF00, 0xFF, 0xFF000000}},
	{ImageFormat::RGBA8, true, {0xFF0000, 0xFF00, 0xFF, 0}}, //Альфа не должна использоваться!
	{ImageFormat::RGB565, true, {0xF800, 0x7E0, 0x1F, 0}},

	{ImageFormat::RGB5, true, {0x7C00, 0x3E0, 0x1F, 0}},
	{ImageFormat::RGB5A1, true, {0x7C00, 0x3E0, 0x1F, 0x8000}},
	{ImageFormat::RGBA4, true, {0xF00, 0xF0, 0xF, 0xF000}},

	{ImageFormat::BGR233, false, {0xE0, 0x1C, 0x3, 0}},
	{ImageFormat::Alpha8, false, {0, 0, 0, 8}},
	//{ D3DFMT_A8R3G3B2,       16, 0xE0,       0x1C,       0x3,        0xFF00 },
	{ImageFormat::RGBX4, true, {0xF00, 0xF0, 0xF, 0}},
	{ImageFormat::RGB10A2, false, {0x3FF, 0xFFC00, 0x3FF00000, 0xC0000000}},
	{ImageFormat::RGBA8, false, {0xFF, 0xFF00, 0xFF0000, 0xFF000000}},
	{ImageFormat::RGB8, false, {0xFF, 0xFF00, 0xFF0000, 0}},
	{ImageFormat::RG16, false, {0xFFFF, 0xFFFF0000, 0, 0}},
	{ImageFormat::RGB10A2, true, {0x3FF00000, 0xFFC00, 0x3FF, 0xC0000000}},

	{ImageFormat::Luminance8, false, {0xFF, 0, 0, 0}},
	{ImageFormat::Luminance16, false, {0xFFFF, 0, 0, 0}},
	{ImageFormat::LuminanceAlpha8, false, {0xFF, 0, 0, 0xFF00}},

	{ImageFormat::Luminance8, false, {0xFF, 0xFF, 0xFF, 0}},
	{ImageFormat::Luminance16, false, {0xFFFF, 0xFFFF, 0xFFFF, 0}},
	{ImageFormat::LuminanceAlpha8, false, {0xFF, 0xFF, 0xFF, 0xFF00}}
};



static ImageFormat GetFormat(const DDS_HEADER& header, const DDS_HEADER_DXT10& dx10header, bool* swapRB)
{
	if(swapRB != nullptr) *swapRB = false;

	if( (header.ddspf.flags & DDPF_FOURCC) &&
		dx10header.resourceDimension != D3D10_RESOURCE_DIMENSION_UNKNOWN)
	{
		unsigned fourcc = to_fourcc(header.ddspf.fourCC);
		if(to_fourcc("DX10"))
			return DxgiToImageFormat(dx10header.dxgiFormat, swapRB);

		for(auto& v: FourCC_ImageFormat) if(v.fourcc == fourcc) return v.format;
	}

	for(auto& fd: D3d9Formats)
	{
		if(fd.masks!=header.ddspf.rgbaBitMasks) continue;
		if(swapRB!=nullptr) *swapRB=fd.swapRB;
		return fd.format;
	}
	return nullptr;
}

static void SetFormat(ImageFormat format, DDS_PIXELFORMAT& pf, DDS_HEADER_DXT10& dx10header, bool swapRB)
{
	const bool isCompressed = format.IsCompressed();
	pf.size = sizeof(DDS_PIXELFORMAT);
	pf.RGBBitCount = format.BitsPerPixel();
	pf.flags = 0;
	if(format.HasLuminance()) pf.flags |= DDPF_LUMINANCE;
	if(format.HasAlpha()) pf.flags |= DDPF_ALPHAPIXELS;
	if(format.HasColor() && !isCompressed) pf.flags |= DDPF_RGB;

	//Для максимальной совместимости пытаемся обойтись без DX10 заголовка, если формат не требует DX10

	//Ищем среди несжатых форматов DX9
	if(!isCompressed) for(auto& fd: D3d9Formats)
	{
		if(fd.format != format.value || fd.swapRB != swapRB) continue;
		pf.rgbaBitMasks = fd.masks;
		FillZeros(pf.fourCC);
		return;
	}

	pf.rgbaBitMasks = {0,0,0,0};

	//Ищем среди сжатых форматов DX9
	pf.flags |= DDPF_FOURCC;
	for(auto& v: FourCC_ImageFormat)
	{
		if(v.format != format.value) continue;
		from_fourcc(pf.fourCC, v.fourcc);
		return;
	}

	//Ищем формат среди форматов DX10
	CopyTo("DX10", pf.fourCC); //Если это не установлено, то DX10 заголовок записывать не надо
	dx10header.dxgiFormat = DxgiFromImageFormat(format, swapRB);
}

static ImageType GetImageTypeFromDX10Header(const DDS_HEADER_DXT10& dx10header)
{
	auto rd = dx10header.resourceDimension;
	if(rd == D3D10_RESOURCE_DIMENSION_TEXTURE1D)
		return ImageType_2D; //В движке 1D текстуры эмулируются через 2D

	if(rd == D3D10_RESOURCE_DIMENSION_TEXTURE2D)
	{
		if(dx10header.miscFlag & DDS_RESOURCE_MISC_TEXTURECUBE)
		{
			if(dx10header.arraySize == 1) return ImageType_Cube;
			return ImageType_CubeArray;
		}

		if(dx10header.arraySize == 1) return ImageType_2D;
		return ImageType_2DArray;
	}

	if(rd == D3D10_RESOURCE_DIMENSION_TEXTURE3D)
		return ImageType_3D;

	return ImageType_End;
}

static ImageType GetImageTypeFromHeaders(const DDS_HEADER& header, const DDS_HEADER_DXT10& dx10header)
{
	if(to_fourcc(header.ddspf.fourCC) == to_fourcc("DX10"))
	{
		if(dx10header.resourceDimension == D3D10_RESOURCE_DIMENSION_UNKNOWN) return ImageType_End;
		return GetImageTypeFromDX10Header(dx10header);
	}

	if(header.caps2 & DDSCAPS2_VOLUME)
		return ImageType_3D;


	if(header.caps2 & DDSCAPS2_CUBEMAP)
	{
		if(header.caps2 & DDSCAPS2_CUBEMAP_ALL_FACES)
			return ImageType_Cube;

		return ImageType_End; //Частичные кубические текстуры не поддерживаются!
	}

	return ImageType_2D;
}

static void SetHeadersImageType(ImageType type, DDS_HEADER& header, DDS_HEADER_DXT10& dx10header)
{
	if(type == ImageType_2D)
	{
		dx10header.resourceDimension = D3D10_RESOURCE_DIMENSION_TEXTURE2D;
		return;
	}

	if(type == ImageType_2DArray)
	{
		dx10header.resourceDimension = D3D10_RESOURCE_DIMENSION_TEXTURE2D;
		header.caps |= DDSCAPS_COMPLEX;
		return;
	}

	if(type == ImageType_3D)
	{
		header.flags |= DDSD_DEPTH;
		header.caps |= DDSCAPS_COMPLEX;
		header.caps2 |= DDSCAPS2_VOLUME;
		dx10header.resourceDimension = D3D10_RESOURCE_DIMENSION_TEXTURE3D;
		return;
	}

	if(type == ImageType_Cube)
	{
		header.caps |= DDSCAPS_COMPLEX;
		header.caps2 |= DDSCAPS2_CUBEMAP;
		header.caps2 |= DDSCAPS2_CUBEMAP_ALL_FACES;
		dx10header.resourceDimension = D3D10_RESOURCE_DIMENSION_TEXTURE2D;
		dx10header.miscFlag |= DDS_RESOURCE_MISC_TEXTURECUBE;
		return;
	}
}

static bool has_dx10_header(const DDS_PIXELFORMAT& ddspf)
{
	return (ddspf.flags & DDPF_FOURCC) &&
		Span("DX10").Take(4)|Equals(SpanOfRaw<char>(ddspf.fourCC, 4));
}

bool LoaderDDS::IsValidHeader(const void* header, size_t headerSize) const
{
	if(headerSize < 4) return false;
	return Span("DDS ").Take(4)|Equals(SpanOfRaw<char>(header, 4));
}

ImageInfo LoaderDDS::GetInfo(IInputStream& stream) const
{
	ImageInfo errResult = {{0,0,0}, nullptr, ImageType_End, 0};

	byte headerSignature[4];
	RawReadTo(stream, headerSignature, 4);

	const bool isDDS = IsValidHeader(headerSignature, 4);
	if(!isDDS) return errResult;

	const DDS_HEADER hdr = RawRead<DDS_HEADER>(stream);
	if(hdr.size != 124) return errResult;

	DDS_HEADER_DXT10 dx10header;
	if(has_dx10_header(hdr.ddspf))
		dx10header = RawRead<DDS_HEADER_DXT10>(stream);
	else dx10header.resourceDimension = D3D10_RESOURCE_DIMENSION_UNKNOWN;

	ImageInfo result;
	result.Size = {hdr.width, hdr.height, Max(uint16(1), uint16(hdr.depth))};
	result.Format = GetFormat(hdr, dx10header, nullptr);
	result.Type = GetImageTypeFromHeaders(hdr, dx10header);
	if(result.Type == ImageType_2DArray) result.Size.z = uint16(dx10header.arraySize);
	result.MipmapCount = 0;
	if(hdr.flags & DDSD_MIPMAPCOUNT)
		result.MipmapCount = Max(short(1), short(hdr.mipMapCount));
	if(result.Format == nullptr ||
		result.Type == ImageType_End ||
		result.Size.x*result.Size.y*result.Size.z == 0)
		return errResult;
	return result;
}


AnyImage LoaderDDS::Load(IInputStream& stream) const
{
	stream.PopFirstCount(4); //Пропускаем идентификатор, предполагая, что он уже проверен

	const DDS_HEADER header = RawRead<DDS_HEADER>(stream);
	if(header.size != 124)
		return nullptr;

	DDS_HEADER_DXT10 dx10header;

	if(has_dx10_header(header.ddspf))
		dx10header = RawRead<DDS_HEADER_DXT10>(stream);
	else dx10header.resourceDimension = D3D10_RESOURCE_DIMENSION_UNKNOWN;

	bool swapRB;
	ImageInfo iInfo;
	iInfo.Size = {header.width, header.height, Max(1u, header.depth)};
	iInfo.Format = GetFormat(header, dx10header, &swapRB);
	iInfo.Type = GetImageTypeFromHeaders(header, dx10header);
	if(iInfo.Type == ImageType_2DArray) iInfo.Size.z = uint16(dx10header.arraySize);
	iInfo.MipmapCount = 0;
	if(header.flags & DDSD_MIPMAPCOUNT) iInfo.MipmapCount = Max(short(1), short(header.mipMapCount));
	if(iInfo.Format == nullptr || iInfo.Type == ImageType_End || iInfo.Size.x*iInfo.Size.y*iInfo.Size.z == 0)
		return nullptr;

	/*if(!iInfo.format.IsCompressed())
	{
		if(iInfo.size.x*iInfo.format.BytesPerPixel()!=(int)header.pitchOrLinearSize) return;
	}
	else if(Max(1, iInfo.size.x/4)!=(int)header.pitchOrLinearSize) return;*/

	AnyImage result;
	result.SwapRB = swapRB;
	result.Info = iInfo;
	result.LineAlignment = 1;

	if(iInfo.MipmapCount == 0) iInfo.MipmapCount = 1;
	const auto dataSize = iInfo.CalculateFullDataSize(result.LineAlignment);
	result.Data.SetCountUninitialized(index_t(dataSize));
	RawReadTo(stream, result.Data.Data(), index_t(dataSize));
	return result;
}


void LoaderDDS::Save(const AnyImage& img, IOutputStream& stream) const
{
	stream << "DDS ";

	DDS_HEADER header;
	header.size = sizeof(header);
	header.flags = DDSD_WIDTH|DDSD_HEIGHT|DDSD_CAPS|DDSD_PIXELFORMAT;
	header.width = img.Info.Size.x;
	header.height = img.Info.Size.y;
	header.depth = img.Info.Size.z;
	header.mipMapCount = uint32(img.Info.MipmapCount == 0? short(1): img.Info.MipmapCount);
	if(!img.Info.Format.IsCompressed()) header.flags |= DDSD_PITCH;
	else header.flags |= DDSD_LINEARSIZE;
	header.caps = DDSCAPS_TEXTURE;
	header.caps4 = header.caps3 = header.caps2 = 0;
	CopyTo(header.reserved1).From(Repeat(0));
	header.reserved2 = 0;

	DDS_HEADER_DXT10 dx10header;
	FillZeros(dx10header.unused1);
	FillZeros(dx10header.unused2);
	dx10header.miscFlag = dx10header.miscFlags2 = 0;
	dx10header.arraySize = img.Info.Size.z;
	if(img.Info.Type == ImageType_Cube)
		dx10header.arraySize /= 6;

	SetFormat(img.Info.Format, header.ddspf, dx10header, img.SwapRB);
	SetHeadersImageType(img.Info.Type, header, dx10header);
	if(img.Info.Type==ImageType_2DArray)
	{
		dx10header.arraySize = img.Info.Size.z;
		header.depth = 1;
	}

	if(img.Info.MipmapCount>1)
	{
		header.flags |= DDSD_MIPMAPCOUNT;
		header.caps |= DDSCAPS_MIPMAP|DDSCAPS_COMPLEX;
	}

	/*if(!iInfo.format.IsCompressed())
	{
		if(iInfo.size.x*iInfo.format.BytesPerPixel()!=(int)header.pitchOrLinearSize) return;
	}
	else if(Max(1, iInfo.size.x/4)!=(int)header.pitchOrLinearSize) return;*/

	if(img.Info.Format.IsCompressedBC1_BC7())
	{
		unsigned blockSize = img.Info.Format.BitsPerPixel()*4u*4u/8u; //4*4 верно не для всех сжатых форматов, но верно для BC1-BC7
		header.pitchOrLinearSize = Max(1u, ((img.Info.Size.x + 3u)/4u)) * blockSize;
	}
	else header.pitchOrLinearSize = (unsigned(img.Info.Size.x)*img.Info.Format.BitsPerPixel() + 7u) / 8u;

	RawWrite<DDS_HEADER>(stream, header);
	if(has_dx10_header(header.ddspf))
		RawWrite<DDS_HEADER_DXT10>(stream, dx10header);
	size_t dataSize = img.Info.CalculateFullDataSize(img.LineAlignment);
	INTRA_ASSERT_EQUALS(dataSize, size_t(img.Data.Count()));

	RawWriteFrom(stream, img.Data.Take(index_t(dataSize)));
}

INTRA_IGNORE_WARN_GLOBAL_CONSTRUCTION
const LoaderDDS LoaderDDS::Instance;
} INTRA_END

#endif
