#include "Image/Bindings/DXGI_Formats.h"
#include "Image/ImageFormat.h"
#include "Algo/Mutation/Fill.h"
#include "Range/Generators/ArrayRange.h"

namespace Intra { namespace Image {

static const ImageFormat dxgiFormatConvertTable[]=
{
	null,
	null, ImageFormat::RGBA32f, ImageFormat::RGBA32u, ImageFormat::RGBA32i,
	null, ImageFormat::RGB32f,  ImageFormat::RGB32u,  ImageFormat::RGB32i,
	null, ImageFormat::RGBA16f, ImageFormat::RGBA16, ImageFormat::RGBA16u, ImageFormat::RGBA16s, ImageFormat::RGBA16i,
	null, ImageFormat::RG32f, ImageFormat::RG32u, ImageFormat::RG32i,
	null, ImageFormat::Depth32fStencil8, null, null,
	null, ImageFormat::RGB10A2, ImageFormat::RGB10A2u,
	ImageFormat::R11G11B10f,
	null, ImageFormat::RGBA8, ImageFormat::sRGB8_A8, ImageFormat::RGBA8u, ImageFormat::RGBA8s, ImageFormat::RGBA8i,
	null, ImageFormat::RG16f, ImageFormat::RG16, ImageFormat::RG16u, ImageFormat::RG16s, ImageFormat::RG16i,
	null, ImageFormat::Depth32f, ImageFormat::Red32f, ImageFormat::Red32u, ImageFormat::Red32i,
	null, ImageFormat::Depth24Stencil8, null, null,
	null, ImageFormat::RG8, ImageFormat::RG8u, ImageFormat::RG8s, ImageFormat::RG8i,
	null, ImageFormat::Red16f, ImageFormat::Depth16, ImageFormat::Red16, ImageFormat::Red16u, ImageFormat::Red16s, ImageFormat::Red16i,
	null, ImageFormat::Red8, ImageFormat::Red8u, ImageFormat::Red8s, ImageFormat::Red8i, ImageFormat::Alpha8,
	null, ImageFormat::RGB9E5, null, null,

	null, ImageFormat::DXT1_RGB, ImageFormat::DXT1_sRGB,
	null, ImageFormat::DXT3_RGBA, ImageFormat::DXT3_sRGB_A,
	null, ImageFormat::DXT5_RGBA, ImageFormat::DXT5_sRGB_A,
	null, ImageFormat::RGTC_Red, ImageFormat::RGTC_SignedRed,
	null, ImageFormat::RGTC_RG, ImageFormat::RGTC_SignedRG,

	ImageFormat::RGB565, ImageFormat::RGB5A1,
	ImageFormat::RGBA8,
	ImageFormat::RGBA8, //Альфа не должна использоваться!
	null,
	null, ImageFormat::sRGB8_A8,
	null, ImageFormat::sRGB8_A8, //Альфа не должна использоваться!

	null, ImageFormat::BPTC_RGBuf, ImageFormat::BPTC_RGBf,
	null, ImageFormat::BPTC_RGBA, ImageFormat::BPTC_sRGB_A,

	null, null, null, null, null, null, null, null, null, null, null, null, null, null, null,
	ImageFormat::RGBA4
};
INTRA_CHECK_TABLE_SIZE(dxgiFormatConvertTable, DXGI_FORMAT_B4G4R4A4_UNORM+1);

ImageFormat DXGI_ToImageFormat(DXGI_FORMAT fmt, bool* swapRB)
{
	if(swapRB!=null) *swapRB = (
		(fmt>=DXGI_FORMAT_B5G6R5_UNORM && fmt<=DXGI_FORMAT_B8G8R8X8_UNORM_SRGB) ||
		fmt==DXGI_FORMAT_B4G4R4A4_UNORM);
	if(fmt>=Meta::NumOf(dxgiFormatConvertTable)) return null;
	return dxgiFormatConvertTable[fmt];
}

DXGI_FORMAT DXGI_FromImageFormat(ImageFormat fmt, bool swapRB)
{
	static DXGI_FORMAT mapNoSwap[ImageFormat::End], mapSwap[ImageFormat::End];
	static bool inited = false;
	if(!inited)
	{
		Algo::FillZeros(mapNoSwap);
		Algo::FillZeros(mapSwap);
		for(int i=DXGI_FORMAT_UNKNOWN+1; i<=DXGI_FORMAT_B4G4R4A4_UNORM; i++)
		{
			const auto dxgi = DXGI_FORMAT(i);
			bool swap = false;
			auto f = DXGI_ToImageFormat(dxgi, &swap).value;
			if(!swap) mapNoSwap[f] = dxgi;
			else mapSwap[f] = dxgi;
		}
		inited = true;
	}
	return (swapRB? mapSwap: mapNoSwap)[fmt.value];
}

}}
