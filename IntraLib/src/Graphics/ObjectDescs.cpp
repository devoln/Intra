#include "Graphics/ObjectDescs.h"

namespace Intra { namespace Graphics {

SamplerCompactDesc::SamplerCompactDesc(const SamplerDesc& desc)
{
	v1 = (uint)desc.MinFilter;
	v1 |= ((uint)desc.MagFilter & 3) << 5;
	v1 |= ((uint)desc.MipFilter & 3) << 7;
	v1 |= ((uint)desc.WrapS & 7) << 9;
	v1 |= ((uint)desc.WrapT & 7) << 12;
	v1 |= ((uint)desc.WrapR & 7) << 15;

	INTRA_ASSERT(desc.MaxMip>=fixed8(0.0) && desc.MaxMip<=fixed8(15.0));
	INTRA_ASSERT(desc.MinMip>=fixed8(0.0) && desc.MinMip<=fixed8(15.0));
	INTRA_ASSERT(desc.MaxMip>=desc.MinMip);
	v1 |= uint(desc.MaxMip*8) << 18;
	v1 |= uint(desc.MinMip*8) << 25;

	INTRA_ASSERT(desc.MipBias>=sfixed8(-7.0) && desc.MipBias<=sfixed8(7.0));
	v2 = (uint)((desc.MipBias+7)*16);
	v2 |= ((uint)desc.DepthCompareMode & 3) << 10;
	v2 |= ((uint)desc.DepthCompareFunc & 15) << 12;
}



const RasterizationStateDesc RasterizationStateDesc::Default =
{
	false, false,
	PolyMode::Fill,
	CullMode::Back,
	Winding::CCW,
	false, 0.0f, 0.0f, 0.0f,
	1.0f
};


}}
