#include "Imaging/Image.h"
#include "Imaging/Bindings/DXGI_Formats.h"

namespace Intra {

using namespace Math;
using namespace IO;

struct DDS_PIXELFORMAT
{
	uintLE size;
	uintLE flags;
	char fourCC[4];
	uintLE RGBBitCount;
	Vector4<uintLE> rgbaBitMasks;
};

struct DDS_HEADER
{
	uintLE size;
	uintLE flags;
	uintLE height, width;
	uintLE pitchOrLinearSize;
	uintLE depth;
	uintLE mipMapCount;
	uint reserved1[11];
	DDS_PIXELFORMAT ddspf;
	uintLE caps, caps2, caps3, caps4;
	uintLE reserved2;
};

enum {DDSD_CAPS=0x00000001,  DDSD_PIXELFORMAT=0x00001000, DDSD_WIDTH=0x00000004, DDSD_HEIGHT=0x00000002,
	DDSD_PITCH=0x00000008, DDSD_MIPMAPCOUNT=0x00020000, DDSD_LINEARSIZE=0x00080000, DDSD_DEPTH=0x00800000};

enum {DDSCAPS_COMPLEX=0x00000008, DDSCAPS_TEXTURE=0x00001000, DDSCAPS_MIPMAP=0x00400000,
	DDSCAPS2_VOLUME=0x00200000, DDSCAPS2_CUBEMAP=0x00000200};

enum {DDSCAPS2_CUBEMAP_POSITIVEX=0x00000400, DDSCAPS2_CUBEMAP_NEGATIVEX=0x00000800,
	DDSCAPS2_CUBEMAP_POSITIVEY=0x00001000, DDSCAPS2_CUBEMAP_NEGATIVEY=0x00002000,
	DDSCAPS2_CUBEMAP_POSITIVEZ=0x00004000, DDSCAPS2_CUBEMAP_NEGATIVEZ=0x00008000,
	DDSCAPS2_CUBEMAP_ALL_FACES=0x0000FC00};



enum D3D10_RESOURCE_DIMENSION: byte
{ 
  D3D10_RESOURCE_DIMENSION_UNKNOWN,
  D3D10_RESOURCE_DIMENSION_BUFFER,
  D3D10_RESOURCE_DIMENSION_TEXTURE1D, D3D10_RESOURCE_DIMENSION_TEXTURE2D, D3D10_RESOURCE_DIMENSION_TEXTURE3D 
};

struct DDS_HEADER_DXT10
{
	DXGI_FORMAT dxgiFormat;
	byte unused1[3];

	D3D10_RESOURCE_DIMENSION resourceDimension;
	byte unused2[3];

	uintLE miscFlag;
	uintLE arraySize;
	uintLE miscFlags2;
};


enum: uint {
	DDS_RESOURCE_MISC_TEXTURECUBE=4
};

enum: uint {
	DDPF_ALPHAPIXELS=1, DDPF_ALPHA=2, DDPF_FOURCC=4, DDPF_RGB=0x40, DDPF_LUMINANCE=0x20000
};

inline uint to_fourcc(const char* c)
{
	return uint(c[0])|(uint(c[1]) << 8u)|(uint(c[2]) << 16u)|(uint(c[3]) << 24u);
}

inline void from_fourcc(char* dst, uint f)
{
	dst[0] = char(f & 255);
	dst[1] = char((f >> 8) & 255);
	dst[2] = char((f >> 16) & 255);
	dst[3] = char((f >> 24) & 255);
}

static const struct {uint fourcc; ImageFormat::I format;} FourCC_ImageFormat[]={
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
	if(swapRB!=null) *swapRB=false;

	if( (header.ddspf.flags & DDPF_FOURCC) && dx10header.resourceDimension!=D3D10_RESOURCE_DIMENSION_UNKNOWN)
	{
		uint fourcc = to_fourcc(header.ddspf.fourCC);
		if(to_fourcc("DX10"))
			return DXGI_ToImageFormat(dx10header.dxgiFormat, swapRB);

		for(auto& v: FourCC_ImageFormat) if(v.fourcc==fourcc) return v.format;
	}

	for(auto& fd: D3d9Formats)
	{
		if(fd.masks!=header.ddspf.rgbaBitMasks) continue;
		if(swapRB!=null) *swapRB=fd.swapRB;
		return fd.format;
	}
	return null;
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
		if(fd.format!=format.value || fd.swapRB!=swapRB) continue;
		pf.rgbaBitMasks = fd.masks;
		Algo::FillZeros(pf.fourCC);
		return;
	}

	pf.rgbaBitMasks = {0,0,0,0};

	//Ищем среди сжатых форматов DX9
	pf.flags |= DDPF_FOURCC;
	for(auto& v: FourCC_ImageFormat)
	{
		if(v.format!=format.value) continue;
		from_fourcc(pf.fourCC, v.fourcc);
		return;
	}

	//Ищем формат среди форматов DX10
	C::memcpy(pf.fourCC, "DX10", 4); //Если это не установлено, то DX10 заголовок записывать не надо
	dx10header.dxgiFormat = DXGI_FromImageFormat(format, swapRB);
}

static ImageType GetImageTypeFromDX10Header(const DDS_HEADER_DXT10& dx10header)
{
	auto rd = dx10header.resourceDimension;
	if(rd==D3D10_RESOURCE_DIMENSION_TEXTURE1D)
		return ImageType_2D; //В движке 1D текстуры эмулируются через 2D

	if(rd==D3D10_RESOURCE_DIMENSION_TEXTURE2D)
	{
		if(dx10header.miscFlag & DDS_RESOURCE_MISC_TEXTURECUBE)
		{
			if(dx10header.arraySize==1) return ImageType_Cube;
			return ImageType_CubeArray;
		}

		if(dx10header.arraySize==1) return ImageType_2D;
		return ImageType_2DArray;
	}

	if(rd==D3D10_RESOURCE_DIMENSION_TEXTURE3D)
		return ImageType_3D;

	return ImageType_End;
}

static ImageType GetImageTypeFromHeaders(const DDS_HEADER& header, const DDS_HEADER_DXT10& dx10header)
{
	if(to_fourcc(header.ddspf.fourCC)==to_fourcc("DX10"))
	{
		if(dx10header.resourceDimension==D3D10_RESOURCE_DIMENSION_UNKNOWN) return ImageType_End;
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

void SetHeadersImageType(ImageType type, DDS_HEADER& header, DDS_HEADER_DXT10& dx10header)
{
	if(type==ImageType_2D)
	{
		dx10header.resourceDimension = D3D10_RESOURCE_DIMENSION_TEXTURE2D;
		return;
	}

	if(type==ImageType_2DArray)
	{
		dx10header.resourceDimension = D3D10_RESOURCE_DIMENSION_TEXTURE2D;
		header.caps |= DDSCAPS_COMPLEX;
		return;
	}

	if(type==ImageType_3D)
	{
		header.flags |= DDSD_DEPTH;
		header.caps |= DDSCAPS_COMPLEX;
		header.caps2 |= DDSCAPS2_VOLUME;
		dx10header.resourceDimension = D3D10_RESOURCE_DIMENSION_TEXTURE3D;
		return;
	}

	if(type==ImageType_Cube)
	{
		header.caps |= DDSCAPS_COMPLEX;
		header.caps2 |= DDSCAPS2_CUBEMAP;
		header.caps2 |= DDSCAPS2_CUBEMAP_ALL_FACES;
		dx10header.resourceDimension = D3D10_RESOURCE_DIMENSION_TEXTURE2D;
		dx10header.miscFlag |= DDS_RESOURCE_MISC_TEXTURECUBE;
		return;
	}
}

bool has_dx10_header(const DDS_PIXELFORMAT& ddspf)
{
	return (ddspf.flags & DDPF_FOURCC) && C::memcmp(ddspf.fourCC, "DX10", sizeof(ddspf.fourCC))==0;
}

ImageInfo pe_get_dds_info(byte header[148])
{
	ImageInfo errResult = {{0,0,0}, null, ImageType_End, 0};

	const bool isDDS = (header[0]=='D' && header[1]=='D' && header[2]=='S' && header[3]==' ');
	if(!isDDS) return errResult;

	const DDS_HEADER& hdr = *reinterpret_cast<DDS_HEADER*>(header+4);
	if(hdr.size!=124) return errResult;

	DDS_HEADER_DXT10 dx10header;
	if(has_dx10_header(hdr.ddspf))
		dx10header = *reinterpret_cast<DDS_HEADER_DXT10*>(header+4+sizeof(DDS_HEADER));
	else dx10header.resourceDimension = D3D10_RESOURCE_DIMENSION_UNKNOWN;

	ImageInfo result;
	result.Size = {hdr.width, hdr.height, Max(ushort(1), ushort(hdr.depth))};
	result.Format = GetFormat(hdr, dx10header, null);
	result.Type = GetImageTypeFromHeaders(hdr, dx10header);
	if(result.Type==ImageType_2DArray) result.Size.z = ushort(dx10header.arraySize);
	result.MipmapCount = 0;
	if(hdr.flags & DDSD_MIPMAPCOUNT)
		result.MipmapCount = Max(ushort(1), ushort(hdr.mipMapCount));
	if(result.Format==null || result.Type==ImageType_End || result.Size.x*result.Size.y*result.Size.z==0)
		return errResult;
	return result;
}


#ifndef INTRA_NO_DDS_LOADER
void Image::loadDDS(IO::IInputStream* s, size_t bytes)
{
	auto startPos = s->GetPos();
	s->Skip(4); //Пропускаем идентификатор, предполагая, что он уже проверен

	DDS_HEADER header = s->Read<DDS_HEADER>();
	if(header.size!=124) return;
	DDS_HEADER_DXT10 dx10header;

	if(has_dx10_header(header.ddspf))
		dx10header = s->Read<DDS_HEADER_DXT10>();
	else dx10header.resourceDimension = D3D10_RESOURCE_DIMENSION_UNKNOWN;

	bool swapRB;
	ImageInfo iInfo;
	iInfo.Size = {header.width, header.height, Max(1u, header.depth)};
	iInfo.Format = GetFormat(header, dx10header, &swapRB);
	iInfo.Type = GetImageTypeFromHeaders(header, dx10header);
	if(iInfo.Type==ImageType_2DArray) iInfo.Size.z = ushort(dx10header.arraySize);
	iInfo.MipmapCount=0;
	if(header.flags & DDSD_MIPMAPCOUNT) iInfo.MipmapCount = Max(ushort(1), ushort(header.mipMapCount));
	if(iInfo.Format==null || iInfo.Type==ImageType_End || iInfo.Size.x*iInfo.Size.y*iInfo.Size.z==0)
	{
		s->SetPos(startPos+bytes);
		return;
	}

	/*if(!iInfo.format.IsCompressed())
	{
		if(iInfo.size.x*iInfo.format.BytesPerPixel()!=(int)header.pitchOrLinearSize) return;
	}
	else if(Max(1, iInfo.size.x/4)!=(int)header.pitchOrLinearSize) return;*/

	SwapRB = swapRB;
	Info = iInfo;
	LineAlignment = 1;

	if(iInfo.MipmapCount==0) iInfo.MipmapCount=1;
	size_t dataSize = iInfo.CalculateFullDataSize(LineAlignment);
	Data.Clear();
	Data.SetCountUninitialized(dataSize);
	s->ReadData(Data.Data(), dataSize);
}


void Image::saveDDS(IO::IOutputStream* s) const
{
	s->WriteData("DDS ", 4);

	DDS_HEADER header;
	header.size = sizeof(header);
	header.flags = DDSD_WIDTH|DDSD_HEIGHT|DDSD_CAPS|DDSD_PIXELFORMAT;
	header.width = Info.Size.x;
	header.height = Info.Size.y;
	header.depth = Info.Size.z;
	header.mipMapCount = Info.MipmapCount==0? ushort(1): Info.MipmapCount;
	if(!Info.Format.IsCompressed()) header.flags |= DDSD_PITCH;
	else header.flags |= DDSD_LINEARSIZE;
	header.caps = DDSCAPS_TEXTURE;
	header.caps4 = header.caps3 = header.caps2 = 0;
	Algo::FillZeros(header.reserved1);
	header.reserved2 = 0;

	DDS_HEADER_DXT10 dx10header;
	Algo::FillZeros(dx10header.unused1);
	Algo::FillZeros(dx10header.unused2);
	dx10header.miscFlag = dx10header.miscFlags2 = 0;
	dx10header.arraySize = Info.Size.z;
	if(Info.Type==ImageType_Cube)
		dx10header.arraySize /= 6;

	SetFormat(Info.Format, header.ddspf, dx10header, SwapRB);
	SetHeadersImageType(Info.Type, header, dx10header);
	if(Info.Type==ImageType_2DArray)
	{
		dx10header.arraySize = Info.Size.z;
		header.depth = 1;
	}

	if(Info.MipmapCount>1)
	{
		header.flags |= DDSD_MIPMAPCOUNT;
		header.caps |= DDSCAPS_MIPMAP|DDSCAPS_COMPLEX;
	}

	/*if(!iInfo.format.IsCompressed())
	{
		if(iInfo.size.x*iInfo.format.BytesPerPixel()!=(int)header.pitchOrLinearSize) return;
	}
	else if(Max(1, iInfo.size.x/4)!=(int)header.pitchOrLinearSize) return;*/

	if(Info.Format.IsCompressedBC1_BC7())
	{
		uint blockSize = Info.Format.BitsPerPixel()*4u*4u/8u; //4*4 верно не для всех сжатых форматов, но верно для BC1-BC7
		header.pitchOrLinearSize = Max(1u, ((Info.Size.x+3u)/4u)) * blockSize;
	}
	else header.pitchOrLinearSize = (uint(Info.Size.x)*Info.Format.BitsPerPixel()+7u)/8u;

	s->Write<DDS_HEADER>(header);
	if(has_dx10_header(header.ddspf)) s->Write<DDS_HEADER_DXT10>(dx10header);
	size_t dataSize = Info.CalculateFullDataSize(LineAlignment);
	s->WriteData(Data.Data(), dataSize);
}
#endif

}
