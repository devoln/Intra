#include "Graphics/Graphics.h"
using namespace math;

Graphics::ContextRef AGraphics::Current=null;

AGraphics::AGraphics(GraphicsWindow* wnd): owner_window(wnd),
	vendor(GpuVendor::Unknown), clamp_sampler(null), white2D1x1(null)
{
	if(Current==null) Current=this;
	wnd->my_graphics = this;
}

AGraphics::Texture* AGraphics::GetWhiteTexture()
{
	if(white2D1x1!=null) return white2D1x1;
	ImageInfo desc;
	desc.Format = ImageFormat::Luminance8;
	desc.MipmapCount=1;
	desc.Size={1,1,1};
	desc.Type = ImageType_2D;
	white2D1x1 = TextureAllocate(desc, false);
	byte whiteColor[4] ={255, 255, 255, 255};
	TextureSetData(white2D1x1, 0, {0,0,0}, {1,1,1}, ImageFormat::Luminance8, whiteColor, false, 1);
	return white2D1x1;
}

void AGraphics::DrawScreenQuad(vec2 pos, vec2 size, vec2 leftTopTexCoord, vec2 rightBottomTexCoord, AGraphics::Texture* tex)
{
	RenderState States;
	States.DepthTestEnabled=false;
	SetupStates(&States);

	if(clamp_sampler==null)
	{
		SamplerDesc samp;
		samp.WrapS=samp.WrapT=samp.WrapR=TexWrap::Clamp;
		clamp_sampler=SamplerCreate(samp);
	}
	TextureBind(tex, clamp_sampler, 0);

	ShaderSetUniform(screen_quad_sh, (uintptr)ShaderUniformGetId(screen_quad_sh, "offset"), 1, ValueType::FVec2, pos);
	ShaderSetUniform(screen_quad_sh, (uintptr)ShaderUniformGetId(screen_quad_sh, "size"), 1, ValueType::FVec2, size);
	ShaderSetUniform(screen_quad_sh, (uintptr)ShaderUniformGetId(screen_quad_sh, "tcStart"), 1, ValueType::FVec2, leftTopTexCoord);
	ShaderSetUniform(screen_quad_sh, (uintptr)ShaderUniformGetId(screen_quad_sh, "tcEnd"), 1, ValueType::FVec2, rightBottomTexCoord);
	DrawCall dc;
	dc.VertexCount=4;
	dc.Type=PrimitiveTopology::TriangleFan;
	Draw(dc, screen_quad_sh, null);
}
