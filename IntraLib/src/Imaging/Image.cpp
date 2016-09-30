#include "Imaging/Image.h"
#include "IO/File.h"
#include "Containers/Array.h"

namespace Intra {

using namespace Math;
using namespace IO;


Image Image::FromData(USVec3 size, ImageFormat format, ImageType type,
	const void* data, ushort borderLeft, ushort borderTop, ushort borderRight, ushort borderBottom)
{
	if(data==null || size.x*size.y*size.z==0 || format==ImageFormat::End || type==ImageType_End)
		return null; //Переданы неверные параметры!

	Image result({size.x+borderLeft+borderRight, size.y+borderTop+borderBottom, 1}, format, 0, type);
	result.Data.SetCountUninitialized(result.Info.CalculateFullDataSize(1));
	if(data==null || borderLeft+borderRight+borderTop+borderBottom!=0)
		core::memset(result.Data.Data(), 0, result.Data.SizeInBytes());

	result.LineAlignment=1;
	for(uint y=borderTop; y<uint(size.y+borderTop); y++)
	{
		const uint dstOffset = y*result.Info.Size.x+borderLeft;
		const uint srcOffset = y*size.x;
		const uint bpp = format.BytesPerPixel();
		const uint lineSize = size.x*bpp;
		core::memcpy(result.Data.Data()+dstOffset*bpp, reinterpret_cast<const byte*>(data)+srcOffset*bpp, lineSize);
	}

	return result;
}


#if INTRA_DISABLED

Image Image::ExtractChannel(char channelName, ImageFormat compatibleFormat, ushort newLineAlignment) const
{
	if(newLineAlignment==0) newLineAlignment=LineAlignment;
	const auto channelType = Info.Format.GetComponentType();

	INTRA_ASSERT(compatibleFormat.GetValueType()==channelType);
	INTRA_ASSERT(compatibleFormat.ComponentCount()==1);

	const auto channelWidth = channelType.Size();
	const auto channelCount = Info.Format.ComponentCount();

	ushort channelIndex = 0;
	if(channelName=='r' || channelName=='x') channelIndex = 0;
	else if(channelName=='g' || channelName=='y') channelIndex = 1;
	else if(channelName=='b' || channelName=='z') channelIndex = 2;
	else if(channelName=='a' || channelName=='w') channelIndex = 3;
	else return null;

	const size_t usefulSrcLineBytes = size_t( Info.Size.x*Info.Format.BytesPerPixel() );
	const size_t usefulDstLineBytes = size_t( Info.Size.x*compatibleFormat.BytesPerPixel() );
	const size_t srcLineBytes = (usefulSrcLineBytes+LineAlignment-1)&~(LineAlignment-1);
	const size_t dstLineBytes = (usefulDstLineBytes+newLineAlignment-1)&~(newLineAlignment-1);
	const size_t srcDataSize = Info.Size.y*srcLineBytes;
	const size_t dstDataSize = Info.Size.y*dstLineBytes;

	Image result(Info.Size, compatibleFormat, Info.MipmapCount, Info.Type);
	result.Data.SetCountUninitialized(dstDataSize);

	INTRA_INTERNAL_ERROR("Not implemented!");

	return result;
}

#endif


const void* Image::GetMipmapDataPtr(size_t mip) const
{
	const byte* ptr = Data.Data();
	for(size_t i=0; i<mip; i++)
		ptr += Info.CalculateMipmapDataSize(i, LineAlignment);
	return ptr;
}

Array<const void*> Image::GetMipmapPointers() const
{
	ushort mc = Max<ushort>(Info.MipmapCount, ushort(1));
	Array<const void*> result(mc);
	for(ushort i=0; i<mc; i++) result.AddLast(GetMipmapDataPtr(i));
	return result;
}

#ifndef INTRA_NO_IMAGE_LOADING


Image Image::FromFile(StringView filename)
{
	DiskFile::Reader file(filename);
	if(file==null) return null;
	return FromStream(&file, size_t(file.GetSize()));
}

void Image::SaveToFileDDS(StringView filename) const
{
	DiskFile::Writer file(filename);
	if(file==null) return;
	return ToStream(&file);
}

//Загрузить изображение из файла FileName
void Image::load(IO::IInputStream* s, size_t bytes)
{
	INTRA_ASSERT(s->IsSeekable());
	auto startPos=s->GetPos();
	byte header[12];
	s->ReadData(header, sizeof(header));
	s->SetPos(startPos);
	auto fmt = DetectFileFormatByHeader(header);
	decltype(&Image::load) loaders[] = {
		&Image::load_with_library,

#ifdef INTRA_IMAGE_BMP_LOADER
		&Image::loadBMP,
#else
		&Image::load_with_library,
#endif
		&Image::load_with_library,
		&Image::load_with_library,
#ifndef INTRA_IMAGE_DDS_LOADER
		&Image::loadDDS,
#else
		null,
#endif
		&Image::load_with_library,
#ifndef INTRA_NO_TGA_LOADER
		&Image::loadTGA,
#else
		null,
#endif
#ifndef INTRA_NO_KTX_LOADER
		&Image::loadKTX,
#else
		null,
#endif
	};
	INTRA_CHECK_TABLE_SIZE(loaders, FileFormat::Unknown);
	(this->*loaders[size_t(fmt)])(s, bytes);
	INTRA_ASSERT(s->GetPos()==startPos+bytes);
	s->SetPos(startPos+bytes);
}

//Определение формата файла по заголовку
inline bool is_valid_jpeg_header(byte header[2]) {return header[0]==0xFF && header[1]==0xD8;}
inline bool is_valid_bmp_header(byte header[2]) {return header[0]=='B' && header[1]=='M';}
inline bool is_valid_gif_header(byte header[3]) {return header[0]=='G' && header[1]=='I' && header[2]=='F';}
inline bool is_valid_tiff_header(byte header[3]) {return header[0]=='I' && header[1]=='I' && header[2]=='*';}
inline bool is_valid_dds_header(byte header[4]) {return header[0]=='D' && header[1]=='D' && header[2]=='S' && header[3]==' ';}

static bool is_valid_png_header(byte header[8])
{
	static const byte pngSignature[]={137, 'P', 'N', 'G', 13, 10, 26, 10};
	return core::memcmp(header, pngSignature, sizeof(pngSignature))==0;
}

static bool is_valid_tga_header(byte header[12])
{
	if((header[0]|header[1]|header[3]|header[4]|header[5]|header[6]|header[8]|header[9]|header[10]|header[11])!=0) return false;
	if(header[2]!=10 && header[2]!=2) return false; //Поддерживаются пока только форматы 2 (несжатый) и 10 (RLE)
	return true;
}

static bool is_valid_ktx_header(byte header[12])
{
	static const byte fileIdentifier[12] = {0xAB, 0x4B, 0x54, 0x58, 0x20, 0x31, 0x31, 0xBB, 0x0D, 0x0A, 0x1A, 0x0A};
	return core::memcmp(header, fileIdentifier, sizeof(fileIdentifier))==0;
}

Image::FileFormat Image::DetectFileFormatByHeader(byte header[12])
{
	decltype(&is_valid_jpeg_header) validators[] = {
		is_valid_jpeg_header, is_valid_bmp_header,
		is_valid_gif_header, is_valid_tiff_header,
		is_valid_dds_header, is_valid_png_header,
		is_valid_tga_header, is_valid_ktx_header
	};
	INTRA_CHECK_TABLE_SIZE(validators, FileFormat::Unknown);

	for(ushort i=0; i<ushort(FileFormat::Unknown); i++)
		if(validators[i]!=null && validators[i](header)) return FileFormat(i);

	return FileFormat::Unknown;
}



#if(INTRA_MINEXE<=1)
//Получение информации о изображении по сырому массиву байт
static ImageInfo get_jpg_info(IInputStream* s)
{
	ImageInfo result={{0,0,1}, null, ImageType_2D, 0};
	byte SOI[2]; s->ReadData(SOI, sizeof(SOI));
	if(!is_valid_jpeg_header(SOI)) return result;
	while(!s->EndOfStream())
	{
		s->Skip(1);
		byte chunkName = s->Read<byte>();
		ushort chunkSize = ushort(s->Read<ushortBE>()-2u);
		if(chunkName==0xC0 || chunkName==0xC2) // baseline/progressive (huffman)
		{
			s->Skip(1); // precision
			result.Size.y = s->Read<ushortBE>();
			result.Size.x = s->Read<ushortBE>();
			byte bpp = s->Read<byte>();
			if(bpp==3) result.Format = ImageFormat::RGB8;
			else if(bpp==1) result.Format = ImageFormat::Luminance8;
			break;
		}
		s->Skip(chunkSize);
	}
	return result;
}

static ImageInfo get_png_info(byte header[26])
{
	auto data = header+8; //Пропускаем сигнатуру
	data += 2*sizeof(intBE); //Переходим к данным блока
	struct
	{
		Vector2<uintBE> size;
		byte bitsPerComponent, colorType;
	} ihdrPart;
	core::memcpy(&ihdrPart, data, sizeof(ihdrPart));

	ImageFormat fmt = null;
	if(ihdrPart.bitsPerComponent==8)
	{
		if(ihdrPart.colorType==0) fmt = ImageFormat::Luminance8;
		else if(ihdrPart.colorType==4) fmt = ImageFormat::LuminanceAlpha8;
		else if(ihdrPart.colorType==2) fmt = ImageFormat::RGB8;
		else if(ihdrPart.colorType==6) fmt = ImageFormat::RGBA8;
	}
	else if(ihdrPart.bitsPerComponent==16)
	{
		if(ihdrPart.colorType==0) fmt = ImageFormat::Luminance16;
		//else if(ihdrPart.colorType==4) fmt = ImageFormat::LuminanceAlpha16;
		else if(ihdrPart.colorType==2) fmt = ImageFormat::RGB16;
		else if(ihdrPart.colorType==6) fmt=ImageFormat::RGBA16;
	}
	return {
		USVec3(ihdrPart.size.x, ihdrPart.size.y, 1),
		fmt, ImageType_2D, 0
	};
}

static ImageInfo get_bmp_info(byte data[30])
{
	struct
	{
		uint infoSize;
		UVec2 size;
		ushort colorPlanes;
		ushort bitsPerPixel;
	} hdrPart;
	core::memcpy(&hdrPart, data+14, sizeof(hdrPart));

	ImageFormat fmt;
	if(hdrPart.bitsPerPixel==32) fmt = ImageFormat::RGBA8;
	else if(hdrPart.bitsPerPixel==24) fmt = ImageFormat::RGB8;
	else if(hdrPart.bitsPerPixel==16) fmt = ImageFormat::RGB5A1;
	else if(hdrPart.bitsPerPixel==8 || hdrPart.bitsPerPixel==4 || hdrPart.bitsPerPixel==1) fmt = ImageFormat::RGBA8;
	else fmt=null;
	return {USVec3(hdrPart.size, 1), fmt, ImageType_2D, 0};
}


ImageInfo pe_get_dds_info(byte header[148]);
ImageInfo pe_get_ktx_info(byte header[64]);
#endif

static ImageInfo get_tga_info(byte header[18])
{
	USVec2 size = {(header[13] << 8)+header[12], (header[15] << 8)+header[14]};
	static const ImageFormat formatsFromComponents[] = {
		ImageFormat::Luminance8, ImageFormat::RG8, ImageFormat::RGB8, ImageFormat::RGBA8};
	ImageFormat format = formatsFromComponents[header[16]/8-1];
	return {
		USVec3(size.x, size.y, 1),
		format, ImageType_2D, 0
	};
}

ImageInfo Image::GetImageInfo(IInputStream* s, Image::FileFormat* format)
{
	INTRA_ASSERT(s->IsSeekable());
	byte fmtHdr[148];
	auto startPos = s->GetPos();
	s->ReadData(fmtHdr, 148);
	s->SetPos(startPos);
#if(INTRA_MINEXE<=1)
	auto fmt = DetectFileFormatByHeader(fmtHdr);
#else
	FileFormat fmt = FileFormat::Unknown;
#endif
	if(format!=null) *format=fmt;
	ImageInfo result = {{0, 0, 1}, null, ImageType_2D, 0};
#if(INTRA_MINEXE<=1)
	switch(fmt)
	{
		case FileFormat::DDS:  result = pe_get_dds_info(fmtHdr); break;
		case FileFormat::KTX:  result = pe_get_ktx_info(fmtHdr); break;
		case FileFormat::BMP:  result = get_bmp_info(fmtHdr);    break;
		case FileFormat::PNG:  result = get_png_info(fmtHdr);    break;
		case FileFormat::TGA:  result = get_tga_info(fmtHdr);    break;
		case FileFormat::JPEG: result = get_jpg_info(s);         break;
		default: return ImageInfo();
	}
#else
	return ImageInfo();
#endif
	s->SetPos(startPos);
	return result;
}

ImageInfo Image::GetImageInfo(StringView filename, Image::FileFormat* format)
{
	IO::DiskFile::Reader file(filename);
	if(file==null) return ImageInfo();
	return GetImageInfo(&file, format);
}




#ifndef INTRA_NO_BMP_LOADER
static uint bit_count_by_mask(uint mask)
{
	uint bitCount=0;
	while(mask!=0) mask &= mask-1, bitCount++;
	return bitCount;
}

static uint bit_position_by_mask(uint mask)
{
	return bit_count_by_mask((mask&(~mask+1))-1);
}

/*static uint component_by_mask(uint color, uint mask)
{
	return (color&mask) >> bit_position_by_mask(mask);
}*/

static uint bit_count_to_mask(uint bitCount)
{
	return (bitCount==32)? 0xFFFFFFFFu: (1u << bitCount)-1u;
}

static uint convert(uint color, uint fromBitCount, uint toBitCount)
{
	if(toBitCount<fromBitCount) color >>= fromBitCount-toBitCount;
	else
	{
		color<<=toBitCount-fromBitCount;
		if(color>0) color |= bit_count_to_mask(toBitCount-fromBitCount);
	}
	return color;
}

template<typename T> static void swap_red_blue_typed(size_t lineUnused, size_t componentCount, USVec2 sizes, T* data)
{
	for(int y=0; y<sizes.y; y++)
	{
		for(int x=0; x<sizes.x; x++)
		{
			core::swap(data[0], data[2]);
			data += componentCount;
		}
		data += lineUnused;
	}
}

static void swap_red_blue(ImageFormat format, ushort lineAlignment, USVec2 sizes, byte* data)
{
	const auto components = format.ComponentCount();
	const auto bytesPerComp = format.GetComponentType().Size();
	const ushort lineUnusedBytes = ushort(lineAlignment-sizes.x*format.BytesPerPixel()%lineAlignment);
	if(bytesPerComp==1) swap_red_blue_typed<byte>(lineUnusedBytes, components, sizes, data);
	else if(bytesPerComp==2) swap_red_blue_typed<ushort>(lineUnusedBytes/2u, components, sizes, reinterpret_cast<ushort*>(data));
	else if(bytesPerComp==4) swap_red_blue_typed<uint>(lineUnusedBytes/4u, components, sizes, reinterpret_cast<uint*>(data));
	else INTRA_INTERNAL_ERROR("swap_red_blue пока не поддерживает пакованные форматы!");
}

static void read_pixel_data_block(IInputStream* s, USVec2 sizes, ImageFormat srcFormat, ImageFormat dstFormat,
	bool swapRB, bool flipVert, ushort srcAlignment, ushort dstAlignment, byte* dstBuf)
{
	INTRA_ASSERT(s->IsSeekable());
	INTRA_ASSERT(srcFormat.ComponentCount()>=3 || !swapRB);
	const size_t usefulSrcLineBytes = size_t(sizes.x*srcFormat.BytesPerPixel());
	const size_t usefulDstLineBytes = size_t(sizes.x*dstFormat.BytesPerPixel());
	const size_t srcLineBytes = (usefulSrcLineBytes+srcAlignment-1u)&~size_t(srcAlignment-1u);
	const size_t dstLineBytes = (usefulDstLineBytes+dstAlignment-1u)&~size_t(dstAlignment-1u);
	const size_t srcDataSize = sizes.y*srcLineBytes, dstDataSize=sizes.y*dstLineBytes;
	if(dstBuf==null) return s->Skip(srcDataSize);

	if(srcFormat==dstFormat && srcLineBytes==dstLineBytes && !swapRB && !flipVert)
		{s->ReadData(dstBuf, srcDataSize); return;}

	byte* pos = dstBuf;
	if(flipVert) pos += dstDataSize-dstLineBytes;

	if(srcFormat==dstFormat)
	for(int y=0; y<sizes.y; y++)
	{
		s->ReadData(pos, usefulSrcLineBytes);
		s->Skip(srcLineBytes-usefulSrcLineBytes);
		//core::memset(pos+usefulSrcLineBytes, 0, dstLineBytes-usefulSrcLineBytes);
		if(flipVert) pos -= dstLineBytes;
		else pos += dstLineBytes;
	}

	if(srcFormat==ImageFormat::A1_BGR5 && dstFormat==ImageFormat::RGB8)
		for(int y=0; y<sizes.y; y++)
		{
			auto pixels = reinterpret_cast<UBVec3*>(pos);
			for(uint x=0; x<sizes.x; x++)
			{
				ushort color = s->Read<ushortLE>();
				*pixels++ = {(color >> 11) << 3, ((color >> 6) & 0x1f) << 3, ((color >> 1) & 0x1f) << 3};
			}
			if(flipVert) pos -= dstLineBytes;
			else pos += dstLineBytes;
			s->Skip(srcLineBytes-usefulSrcLineBytes);
		}

	if(srcFormat==ImageFormat::RGB5A1 && dstFormat==ImageFormat::RGB8)
		for(int y=0; y<sizes.y; y++)
		{
			auto pixels = reinterpret_cast<UBVec3*>(pos);
			for(uint x=0; x<sizes.x; x++)
			{
				ushort color = s->Read<ushortLE>();
				*pixels++ = {((color >> 10) & 0x1f) << 3, ((color >> 5) & 0x1f) << 3, (color & 0x1f) << 3};
			}
			if(flipVert) pos -= dstLineBytes;
			else pos += dstLineBytes;
			s->Skip(srcLineBytes-usefulSrcLineBytes);
		}

	if(srcFormat==ImageFormat::RGBA8 && dstFormat==ImageFormat::RGB8)
		for(int y=0; y<sizes.y; y++)
		{
			auto pixels = reinterpret_cast<UBVec3*>(pos);
			if(swapRB) for(uint x=0; x<sizes.x; x++)
				*pixels++ = s->Read<UBVec4>().swizzle(2,1,0);
			else for(uint x=0; x<sizes.x; x++)
				*pixels++ = s->Read<UBVec4>().xyz;
			if(flipVert) pos -= dstLineBytes;
			else pos += dstLineBytes;
			s->Skip(srcLineBytes-usefulSrcLineBytes);
		}

	if(swapRB) swap_red_blue(dstFormat, dstAlignment, sizes, dstBuf);
}

static void read_paletted_pixel_data_block(IInputStream* s, const byte* palette, ushort bpp, USVec2 sizes,
	ImageFormat format, bool flipVert, ushort srcAlignment, ushort dstAlignment, byte* dstBuf)
{
	INTRA_ASSERT(s->IsSeekable());
	const ushort bytesPerPixel = format.BytesPerPixel();
	const uint usefulSrcLineBytes = uint(sizes.x*bpp)/8u;
	const uint usefulDstLineBytes = uint(sizes.x*bytesPerPixel);
	const uint srcLineBytes = (usefulSrcLineBytes+srcAlignment-1u)&~(srcAlignment-1u);
	const uint dstLineBytes = (usefulDstLineBytes+dstAlignment-1u)&~(dstAlignment-1u);
	const size_t dstDataSize = sizes.x*dstLineBytes;

	byte* pos = dstBuf;
	if(flipVert) pos += dstDataSize-dstLineBytes;

	for(int y=0; y<sizes.y; y++)
	{
		byte* linePos = pos;
		if(bpp==1) for(uint j=0; j<sizes.x; j+=8)
		{
			byte colorIndices = s->Read<byte>();
			for(int k=0; k<8; k++)
			{
				core::memcpy(linePos, palette + ((colorIndices & 0x80) >> 7)*bytesPerPixel, bytesPerPixel);
				colorIndices = byte(colorIndices << 1);
				linePos += bytesPerPixel;
			}
		}
		else if(bpp==4) for(uint j=0; j<sizes.x; j+=2)
		{
			byte colorIndices = s->Read<byte>();
			core::memcpy(linePos, palette + (colorIndices >> 4)*bytesPerPixel, bytesPerPixel);
			linePos += bytesPerPixel;
			core::memcpy(linePos, palette + (colorIndices & 15)*bytesPerPixel, bytesPerPixel);
			linePos += bytesPerPixel;
		}
		else if(bpp==8) for(uint j=0; j<sizes.x; j++)
		{
			byte colorIndex = s->Read<byte>();
			core::memcpy(linePos, palette + colorIndex*bytesPerPixel, bytesPerPixel);
			linePos+=bytesPerPixel;
		}
		if(!flipVert) pos += dstLineBytes;
		else pos -= dstLineBytes;
		s->Skip(srcLineBytes-usefulSrcLineBytes);
	}
}

void Image::loadBMP(IInputStream* s, size_t bytes)
{
	(void)bytes;

	struct BitmapHeader
	{
		Vector2<intLE> sizes;
		ushortLE planes;
		ushortLE bitCount;
		uintLE Compression;
		uintLE SizeImage;
		intLE PelsPerMeterX;
		intLE PelsPerMeterY;
		uintLE clrUsed;
		uintLE clrImportant;
		Vector4<uintLE> RgbaMasks;
		uintLE CsType;
		uintLE Endpoints[9];
		Vector3<uintLE> GammaRGB;
	} bmpHdr;

	s->Skip(2); //Предполагается, что идентификатор формата уже проверен

	uint fileSize = s->Read<uintLE>();
	(void)fileSize;
	s->Read<uint>();
	const uint dataPos = s->Read<uintLE>();

	const uint hdrSize = s->Read<uintLE>();
	s->ReadData(&bmpHdr, hdrSize-sizeof(uintLE));
	if(bmpHdr.Compression!=0 && bmpHdr.Compression!=3) return; //RLE4, RLE8 и встроенный jpeg\png не поддерживается!

	//Load Color Table

	s->SetPos(14+hdrSize);

	//const uint colorTableSize=(bmpHdr.bitCount>8 || bmpHdr.bitCount==0)? 0: (1 << bmpHdr.bitCount);

	UBVec4 colorTable[256];
	if(bmpHdr.clrUsed!=0)
	{
		s->ReadData(colorTable, sizeof(colorTable[0])*bmpHdr.clrUsed);
		for(uint i=0; i<bmpHdr.clrUsed; i++) colorTable[i]=colorTable[i].swizzle(2,1,0,3);
	}
	else if(bmpHdr.bitCount<=8) //Если палитра отсутствует, то сделаем её сами из оттенков серого. Такие случаи вроде бы не были описаны, но paint создаёт такие bmp
		for(int i=0; i<(1<<bmpHdr.bitCount); i++)
			colorTable[i] = UBVec4(UBVec3(byte( 255*i >> bmpHdr.bitCount )), 255);

	LineAlignment = 1;
	Info.Type = ImageType_2D;
	Info.Size = USVec3(bmpHdr.sizes, 1);
	if(bmpHdr.Compression==0)
	{
		if(bmpHdr.bitCount==1 || bmpHdr.bitCount==4 || bmpHdr.bitCount==8 || bmpHdr.bitCount==32) Info.Format = ImageFormat::RGBA8;
		if(bmpHdr.bitCount==24) Info.Format = ImageFormat::RGB8;
		if(bmpHdr.bitCount==16) Info.Format = ImageFormat::RGB8;
	}
	else Info.Format = ImageFormat::RGBA8;
	Data.SetCountUninitialized(Info.CalculateMipmapDataSize(0, LineAlignment));
	s->SetPos(dataPos);

	if(bmpHdr.Compression==0)
	{
		SwapRB = (bmpHdr.bitCount==24 || bmpHdr.bitCount==32);
		const USVec2 size = {Info.Size.x, Info.Size.y};

		if(bmpHdr.bitCount==16) return read_pixel_data_block(s, size,
			ImageFormat::RGB5A1, Info.Format, false, true, 4, LineAlignment, Data.Data());

		if(bmpHdr.bitCount==24) return read_pixel_data_block(s, size,
			ImageFormat::RGB8,   Info.Format, false, true, 4, LineAlignment, Data.Data());

		if(bmpHdr.bitCount==32) return read_pixel_data_block(s, size,
			ImageFormat::RGBA8,  Info.Format, false, true, 4, LineAlignment, Data.Data());

		read_paletted_pixel_data_block(s, reinterpret_cast<byte*>(colorTable),
				bmpHdr.bitCount, size, Info.Format, true, 4, LineAlignment, Data.Data());
		return;
	}

	//Битовые поля
	//Предполагается, что маски цветовых компонентов могут находиться в любом порядке
	const uint lineWidth = ((uint(Info.Size.x)*bmpHdr.bitCount/8u)+3u)&~3u;
	Array<byte> line;
	line.SetCountUninitialized(lineWidth);
	UVec4 bitCount, bitPositions;
	for(size_t k=0; k<4; k++)
	{
		bitCount[k] = bit_count_by_mask(bmpHdr.RgbaMasks[k]);
		bitPositions[k] = bit_position_by_mask(bmpHdr.RgbaMasks[k]);
	}
	UBVec4* pixels = reinterpret_cast<UBVec4*>(Data.end());

	for(uint i=0; i<Info.Size.y; i++)
	{
		pixels-=Info.Size.x;
		uint index=0;
		s->ReadData(line.Data(), lineWidth);

		byte* linePtr = line.Data();

		for(uint j=0; j<Info.Size.x; j++)
		{
			uint Color=0;
			if(bmpHdr.bitCount==16) Color = *reinterpret_cast<ushortLE*>(linePtr);
			else if(bmpHdr.bitCount==32) Color = *reinterpret_cast<uintLE*>(linePtr);
			else
			{
				// Other formats are not valid
			}
			linePtr+=bmpHdr.bitCount/8;
			for(size_t k=0; k<4; k++)
			{
				uint pixel = convert((Color & bmpHdr.RgbaMasks[k]) >> bitPositions[k], bitCount[k], 8);
				pixels[index][k] = byte(pixel);
			}
			index++;
		}
	}
}
#endif

#ifndef INTRA_NO_TGA_LOADER
//Загрузить изображение из файла в формате tga
void Image::loadTGA(IInputStream* s, size_t bytes)
{
	(void)bytes;

	byte header[18];
	s->ReadData(header, 18);

	const bool compressedRLE = (header[2]==10);

	auto fileInfo = get_tga_info(header);
	if(fileInfo==null) return;

	Info = fileInfo;
	SwapRB = true;
	LineAlignment = compressedRLE? byte(1): byte(4);

	const ushort bytesPerPixel = Info.Format.BytesPerPixel();
	const size_t newSize = Info.CalculateMipmapDataSize(0, LineAlignment);
	Data.Clear();
	Data.SetCountUninitialized(newSize);

	if(!compressedRLE)
	{
		read_pixel_data_block(s, {Info.Size.x, Info.Size.y}, Info.Format, Info.Format, false, true, 4, LineAlignment, Data.Data());
		return;
	}

	const uint pixelcount = uint(Info.Size.x*Info.Size.y);
	uint index=0;
	byte* pos = Data.end() - Info.Size.x*bytesPerPixel;
	for(uint currentPixel=0; currentPixel<pixelcount;)
	{
		int chunkheader = s->Read<byte>();
		if(chunkheader<128)
		{
			chunkheader++;
			for(; chunkheader--!=0; currentPixel++)
			{
				s->ReadData(pos+index*3, bytesPerPixel);
				index++;
				if(index==Info.Size.x) index=0, pos-=Info.Size.x*bytesPerPixel;
			}
			continue;
		}
		chunkheader -= 127;
		byte colorBuffer[4];
		s->ReadData(colorBuffer, bytesPerPixel);
		for(; chunkheader--!=0; currentPixel++)
		{
			core::memcpy(pos+index*3, colorBuffer, bytesPerPixel);
			index++;
			if(index==Info.Size.x) index=0, pos -= Info.Size.x*bytesPerPixel;
		}
	}
}
#endif
}



#if(INTRA_LIBRARY_IMAGE_LOADING==INTRA_LIBRARY_IMAGE_LOADING_Dummy)

namespace Intra {

void Image::load_with_library(IO::IInputStream* s, size_t bytes)
{
	(void)bytes; (void)s;
	INTRA_ASSERT(s!=null);

	Info.Size = {1, 1, 1};
	LineAlignment = 1;
	Info.Format = ImageFormat::RGBA8;
	SwapRB = false;
	Info.Type = ImageType_2D;
	Info.MipmapCount = 1;

	const byte white[4] = {0xFF, 0xFF, 0xFF, 0xFF};
	Data.Clear();
	Data.AddLastRange(AsRange(white));
}

}

#elif(INTRA_LIBRARY_IMAGE_LOADING==INTRA_LIBRARY_IMAGE_LOADING_STB)

#error "INTRA_LIBRARY_IMAGE_LOADING_STB is not implemented!"

#elif(INTRA_LIBRARY_IMAGE_LOADING==INTRA_LIBRARY_IMAGE_LOADING_DevIL)

#include <IL/il.h>
#pragma comment(lib, "DevIL.lib")

namespace Intra {

//Загрузить изображение из BMP, JPG или GIF файла
void Image::load_with_library(IO::IInputStream* s, size_t bytes)
{
	ilInit();
	auto handle = ilGenImage();
	ilBindImage(handle);
	ilLoadImage(file.CStr()); //INVALID_EXTENSION
	Info.Size = {ilGetInteger(IL_IMAGE_WIDTH), ilGetInteger(IL_IMAGE_HEIGHT), 1};
	const ushort glformat = (ushort)ilGetInteger(IL_IMAGE_FORMAT);
	SwapRB = false;
	switch(glformat)
	{
	case IL_LUMINANCE: Info.Format = ImageFormat::Luminance8; break;
	case IL_BGR: SwapRB=true;
	case IL_RGB: Info.Format=ImageFormat::RGB8; break;
	case IL_BGRA: SwapRB=true;
	case IL_RGBA: Info.Format=ImageFormat::RGBA8; break;
	}
	Info.Type = ImageType_2D;
	Data.SetBounds(0); //Оптимизация: чтобы при растягивании буфера не копировалось старое содержимое, которое нам не нужно
	Data.SetBounds(GetSize());
	core::memcpy(Data.First, ilGetData(), GetSize());
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
#include <gdiplus.h> //Поддерживает BMP, GIF, JPEG, PNG, TIFF, Exif, WMF, и EMF. Не работает в WinRT \ Windows Phone

#ifdef _MSC_VER
#pragma warning(pop)
#pragma comment(lib, "gdiplus.lib")
#endif

namespace Intra {

//Загрузить изображение из BMP, JPG или GIF файла
void Image::load_with_library(IO::IInputStream* s, size_t bytes)
{
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

	auto glob=GlobalAlloc(0, bytes);
	if(glob==null) return;
	void* raw=GlobalLock(glob);
	s->ReadData(raw, bytes);
	IStream* stream;
	if(FAILED(CreateStreamOnHGlobal(glob, true, &stream))) return;
	GpBitmap* bmp; GdipCreateBitmapFromStream(stream, &bmp);
	stream->Release();

	//GdipImageRotateFlip(bmp, Gdiplus::RotateNoneFlipY);

	uint width, height; GdipGetImageWidth(bmp, &width); GdipGetImageHeight(bmp, &height);
	Info.Size={width, height, 1};
	LineAlignment=1;

	PixelFormat format; GdipGetImagePixelFormat(bmp, &format);
	if(format==PixelFormat32bppARGB)
		Info.Format = ImageFormat::RGBA8;
	else if(format==PixelFormat32bppRGB || format==PixelFormat24bppRGB)
	{
		Info.Format = ImageFormat::RGB8;
		format = PixelFormat24bppRGB;
	}
	else if(format==PixelFormat16bppARGB1555 || format==PixelFormat16bppRGB555)
	{
		Info.Format = ImageFormat::RGB5A1;
		format = PixelFormat16bppARGB1555;
	}
	else if(format==PixelFormat16bppRGB565)
		Info.Format = ImageFormat::RGB565;
	else if(format & PixelFormatIndexed)
	{
		Info.Format = ImageFormat::RGBA8;
		format = PixelFormat32bppARGB;
	}
	else if(format==PixelFormat16bppGrayScale)
		Info.Format = ImageFormat::Luminance16;
	else if(format & PixelFormatAlpha)
	{
		Info.Format = ImageFormat::RGBA8;
		format = PixelFormat32bppARGB;
	}
	else
	{
		Info.Format = ImageFormat::RGB8;
		format = PixelFormat24bppRGB;
	}

	SwapRB = true;
	Info.Type = ImageType_2D;

	Data.SetCountUninitialized(Info.CalculateMipmapDataSize(0, LineAlignment));
	Data.TrimExcessCapacity();

	BitmapData data = {Info.Size.x, Info.Size.y, Info.Size.x*Info.Format.BytesPerPixel(), format, Data.Data(), 0};
	GdipBitmapLockBits(bmp, null, ImageLockModeRead|ImageLockModeUserInputBuf, format, &data);
	GdipDisposeImage(bmp);
}

}

#elif(INTRA_LIBRARY_IMAGE_LOADING==INTRA_LIBRARY_IMAGE_LOADING_Qt)

#include <QtGui/QImage>

namespace Intra {

//Загрузить изображение из BMP, JPG, PNG или GIF файла
void Image::load_with_library(IInputStream* s, size_t bytes)
{
	const auto startPos = s->GetPos();

	ByteBuffer buf;
	buf.Extend(bytes);
	s->ReadData(buf.Data(), bytes);

	QImage img; img.loadFromData(buf.Data(), bytes);
	Size={img.width(), img.height()};
	const auto qtformat = img.format();
	switch(qtformat)
	{
	case QImage::Format_ARGB32: case QImage::Format_RGB32:
		Info.Format = ImageFormat::RGBA8;
		SwapRB = true;
		break;

	case QImage::Format_RGB888:
		Info.Format = ImageFormat::RGB8;
		SwapRB = true;
		break;

	case QImage::Format_Indexed8:
		Info.Format = ImageFormat::Luminance8;
		SwapRB = false;
		break;

	default:
		s->SetPos(startPos+bytes);
		return;
	}

	Info.Type = ImageType_2D;
	Data.Clear();
	const size_t sizeInBytes = Info.CalculateFullDataSize(1);
	Data.SetCountUninitialized(sizeInBytes);
	core::memcpy(Data.Data(), img.bits(), calculate_size());
}

}

#elif(INTRA_LIBRARY_IMAGE_LOADING==INTRA_LIBRARY_IMAGE_LOADING_SDL)



#elif(INTRA_LIBRARY_IMAGE_LOADING==INTRA_LIBRARY_IMAGE_LOADING_libpng_jpg)



#endif

#endif

