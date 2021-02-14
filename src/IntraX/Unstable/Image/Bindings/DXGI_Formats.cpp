#include "IntraX/Unstable/Image/Bindings/DXGI_Formats.h"
#include "IntraX/Unstable/Image/ImageFormat.h"

#include "Intra/Range/Span.h"

namespace Intra { INTRA_BEGIN
static constexpr ImageFormat dxgiFormatConvertTable[] =
{
	nullptr,
	nullptr, ImageFormat::RGBA32f, ImageFormat::RGBA32u, ImageFormat::RGBA32i,
	nullptr, ImageFormat::RGB32f,  ImageFormat::RGB32u,  ImageFormat::RGB32i,
	nullptr, ImageFormat::RGBA16f, ImageFormat::RGBA16, ImageFormat::RGBA16u, ImageFormat::RGBA16s, ImageFormat::RGBA16i,
	nullptr, ImageFormat::RG32f, ImageFormat::RG32u, ImageFormat::RG32i,
	nullptr, ImageFormat::Depth32fStencil8, nullptr, nullptr,
	nullptr, ImageFormat::RGB10A2, ImageFormat::RGB10A2u,
	ImageFormat::R11G11B10f,
	nullptr, ImageFormat::RGBA8, ImageFormat::sRGB8_A8, ImageFormat::RGBA8u, ImageFormat::RGBA8s, ImageFormat::RGBA8i,
	nullptr, ImageFormat::RG16f, ImageFormat::RG16, ImageFormat::RG16u, ImageFormat::RG16s, ImageFormat::RG16i,
	nullptr, ImageFormat::Depth32f, ImageFormat::Red32f, ImageFormat::Red32u, ImageFormat::Red32i,
	nullptr, ImageFormat::Depth24Stencil8, nullptr, nullptr,
	nullptr, ImageFormat::RG8, ImageFormat::RG8u, ImageFormat::RG8s, ImageFormat::RG8i,
	nullptr, ImageFormat::Red16f, ImageFormat::Depth16, ImageFormat::Red16, ImageFormat::Red16u, ImageFormat::Red16s, ImageFormat::Red16i,
	nullptr, ImageFormat::Red8, ImageFormat::Red8u, ImageFormat::Red8s, ImageFormat::Red8i, ImageFormat::Alpha8,
	nullptr, ImageFormat::RGB9E5, nullptr, nullptr,

	nullptr, ImageFormat::DXT1_RGB, ImageFormat::DXT1_sRGB,
	nullptr, ImageFormat::DXT3_RGBA, ImageFormat::DXT3_sRGB_A,
	nullptr, ImageFormat::DXT5_RGBA, ImageFormat::DXT5_sRGB_A,
	nullptr, ImageFormat::RGTC_Red, ImageFormat::RGTC_SignedRed,
	nullptr, ImageFormat::RGTC_RG, ImageFormat::RGTC_SignedRG,

	ImageFormat::RGB565, ImageFormat::RGB5A1,
	ImageFormat::RGBA8,
	ImageFormat::RGBA8, //Альфа не должна использоваться!
	nullptr,
	nullptr, ImageFormat::sRGB8_A8,
	nullptr, ImageFormat::sRGB8_A8, //Альфа не должна использоваться!

	nullptr, ImageFormat::BPTC_RGBuf, ImageFormat::BPTC_RGBf,
	nullptr, ImageFormat::BPTC_RGBA, ImageFormat::BPTC_sRGB_A,

	nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
	ImageFormat::RGBA4
};
INTRA_CHECK_TABLE_SIZE(dxgiFormatConvertTable, DxgiFormat::B4G4R4A4_UNORM+1);

ImageFormat DxgiToImageFormat(DxgiFormat fmt, bool* oSwapRB)
{
	if(oSwapRB != nullptr) *oSwapRB = (
		(fmt >= DxgiFormat::B5G6R5_UNORM && fmt <= DxgiFormat::B8G8R8X8_UNORM_SRGB) ||
		fmt == DxgiFormat::B4G4R4A4_UNORM);
	if(fmt >= LengthOf(dxgiFormatConvertTable)) return nullptr;
	return dxgiFormatConvertTable[fmt];
}

DxgiFormat DxgiFromImageFormat(ImageFormat fmt, bool swapRB)
{
	static DxgiFormat mapNoSwap[ImageFormat::End] = {};
	static DxgiFormat mapSwap[ImageFormat::End] = {};
	static bool inited = false;
	if(!inited)
	{
		for(int i=DxgiFormat::UNKNOWN+1; i<=DxgiFormat::B4G4R4A4_UNORM; i++)
		{
			const auto dxgi = DxgiFormat(i);
			bool swap = false;
			auto f = DxgiToImageFormat(dxgi, &swap).value;
			if(!swap) mapNoSwap[f] = dxgi;
			else mapSwap[f] = dxgi;
		}
		inited = true;
	}
	return (swapRB? mapSwap: mapNoSwap)[fmt.value];
}
} INTRA_END
