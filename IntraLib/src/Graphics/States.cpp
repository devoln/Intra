#include "Graphics/States.h"
#include "Algo/Search.h"

namespace Intra { namespace Graphics {

template<typename T> T findEnumByName(ArrayRange<const StringView> names, StringView name, T notFoundValue)
{
	const size_t index = Algo::CountUntil(names, name);
	if(index==names.Length()) return notFoundValue;
	return T(index);
}

static const StringView BlendFactor_names[] = {
	"Zero", "One",
	"Src_Color", "InvSrc_Color", "Dst_Color", "InvDst_Color",
	"Src_Alpha", "InvSrc_Alpha", "Dst_Alpha", "InvDst_Alpha",
	"ConstantColor", "InvConstantColor", "ConstantAlpha", "InvConstantAlpha",
	"SrcAlphaSaturate", "Src1Color", "InvSrc1Color", "Src1Alpha", "InvSrc1Alpha"
};

BlendFactor BlendFactor_FromString(StringView name)
{
	INTRA_CHECK_TABLE_SIZE(BlendFactor_names, BlendFactor::End);
	return findEnumByName(BlendFactor_names, name, BlendFactor::End);
}

PolyMode PolyMode_FromString(StringView name)
{
	static const StringView names[] = {"Fill", "Line", "Point"};
	INTRA_CHECK_TABLE_SIZE(names, PolyMode::End);
	return findEnumByName(names, name, PolyMode::End);
}

ShaderType ShaderType_FromString(StringView typeName)
{
	if(typeName=="Vertex") return ShaderType::Vertex;
	else if(typeName=="Fragment" || typeName=="Pixel") return ShaderType::Fragment;
	else if(typeName=="Geometry") return ShaderType::Geometry;
	else if(typeName=="TessControl" || typeName=="TessHull") return ShaderType::TessControl;
	else if(typeName=="TessEval" || typeName=="TessDomain") return ShaderType::TessEval;
	return ShaderType::Null;
}

static const StringView TexFilter_names[]={
	"Nearest", "Linear", "None",
	"AnisotropicX2", "AnisotropicX4", "AnisotropicX8", "AnisotropicX16"
};

TexFilter TexFilter_FromString(StringView name)
{
	INTRA_CHECK_TABLE_SIZE(TexFilter_names, TexFilter::End);
	auto result = findEnumByName(TexFilter_names, name, TexFilter::End);
	if(result==TexFilter::End)
	{
		if(name=="Point") return TexFilter::Nearest;
	}
	return result;
}

static const StringView TexWrap_names[]={"Clamp", "Repeat", "MirroredRepeat"};
TexWrap TexWrap_FromString(StringView name)
{
	INTRA_CHECK_TABLE_SIZE(TexWrap_names, TexWrap::End);
	return findEnumByName(TexWrap_names, name, TexWrap::End);
}

static const StringView CompareFunc_names[]={
	"LessEqual", "GreaterEqual", "Less", "Greater",
	"Equal", "NotEqual", "Always", "Never"
};

CompareFunc CompareFunc_FromString(StringView name)
{
	INTRA_CHECK_TABLE_SIZE(CompareFunc_names, CompareFunc::End);
	return findEnumByName(CompareFunc_names, name, CompareFunc::End);
}

DepthTexCompareMode DepthTexCompareMode_FromString(StringView str)
{
	if(str=="None") return DepthTexCompareMode::None;
	if(str=="CompareRtoTexture") return DepthTexCompareMode::CompareRtoTexture;
	return DepthTexCompareMode::End;
}

}}

