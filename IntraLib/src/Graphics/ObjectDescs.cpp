#include "Graphics/ObjectDescs.h"
#include "Math/Bit.h"

namespace Intra { namespace Graphics {

using Math::BitsOf;

SamplerCompactDesc::SamplerCompactDesc(const SamplerDesc& desc):
	v1(BitsOf(desc.MinFilter) |
		BitsOf(desc.MagFilter, 5) |
		BitsOf(desc.MipFilter, 7) |
		BitsOf(desc.WrapS, 9) |
		BitsOf(desc.WrapT, 12) |
		BitsOf(desc.WrapR, 15) |
		BitsOf(uint(desc.MaxMip*8), 18) |
		BitsOf(uint(desc.MinMip*8), 25)),

	v2(BitsOf(uint((desc.MipBias+7)*16)) |
		BitsOf(desc.DepthCompareMode, 10) |
		BitsOf(desc.DepthCompareFunc, 12))
{
	INTRA_ASSERT(desc.MaxMip>=fixed8(0.0) && desc.MaxMip<=fixed8(15.0));
	INTRA_ASSERT(desc.MinMip>=fixed8(0.0) && desc.MinMip<=fixed8(15.0));
	INTRA_ASSERT(desc.MaxMip>=desc.MinMip);
	INTRA_ASSERT(desc.MipBias>=sfixed8(-7.0) && desc.MipBias<=sfixed8(7.0));
}


}}
