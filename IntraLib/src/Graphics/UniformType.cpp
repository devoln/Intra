#include "Graphics/UniformType.h"
#include "Range/StringView.h"
#include "Containers/String.h"
#include "Containers/HashMap.h"
#include "Imaging/ImagingTypes.h"

namespace Intra { namespace Graphics {

ValueType UniformType::ToValueType() const
{
	if(!IsValid()) return null;
	if(ValueType::IsValid()) return value;
	return ValueType::Int;
}

StringView UniformType::ToStringGLSL() const
{
	static const char* const uniformTypeNameTable[]=
	{
		"sampler1D", "sampler1DArray", "sampler2D", "sampler2DArray", "samplerCube", "samplerCubeArray", "sampler3D", "sampler2DMS", "samplerBuffer",
		"sampler2DShadow", "sampler2DArrayShadow", "samplerCubeShadow", "samplerCubeArrayShadow",
		"isampler1D", "isampler1DArray", "isampler2D", "isampler2DArray", "isamplerCube", "isamplerCubeArray", "isampler3D", "isampler2DMS", "isamplerBuffer",
		"usampler1D", "usampler1DArray", "usampler2D", "usampler2DArray", "usamplerCube", "usamplerCubeArray", "usampler3D", "usampler2DMS", "usamplerBuffer",
			

		"image1D", "image2D", "image3D", "imageCube", "imageBuffer", "image1DArray", "image2DArray", "imageCubeArray", "image2DMS", "image2DMSArray",
		"iimage1D", "iimage2D", "iimage3D", "iimageCube", "iimageBuffer", "iimage1DArray", "iimage2DArray", "iimageCubeArray", "iimage2DMS", "iimage2DMSArray",
		"uimage1D", "uimage2D", "uimage3D", "uimageCube", "uimageBuffer", "uimage1DArray", "uimage2DArray", "uimageCubeArray", "uimage2DMS", "uimage2DMSArray",

		"uniform", "buffer"
	};
	INTRA_CHECK_TABLE_SIZE(uniformTypeNameTable, UniformType::End-ValueType::End);
	if(value<ValueType::End) return ValueType::ToStringGLSL();
	return Intra::StringView(uniformTypeNameTable[value-ValueType::End]);
}

UniformType UniformType::FromStringGLSL(Intra::StringView str)
{
	auto result = ValueType::FromStringGLSL(str);
	if(result!=ValueType::Void) return result;

	static HashMap<Intra::String, UniformType> samplerNameMap;
	if(samplerNameMap==null)
	{
		for(auto i=FirstOfSamplers; i<EndOfSamplers; i=UniformType::I(i+1))
			samplerNameMap.Insert(UniformType(i).ToStringGLSL(), UniformType(i));
	}

	auto found = samplerNameMap.Find(str);
	if(found.Empty()) return UniformType::Void;
	return found.First().Value;
}

ImageType UniformType::GetImageType() const
{
	static const ImageType samplerTable[] = {
		ImageType_1D, ImageType_1DArray, ImageType_2D, ImageType_2DArray, ImageType_Cube, ImageType_CubeArray,
		ImageType_3D, ImageType_2D,
		ImageType_1D, ImageType_2D, ImageType_2DArray, ImageType_Cube, ImageType_CubeArray
	};
	const UniformType::I v = UniformType::I(value);
	if(v>=Sampler1D && v<=SamplerCubeArrayShadow) return samplerTable[v-Sampler1D];
	if(v>=ISampler1D && v<=ISamplerBuffer) return samplerTable[v-ISampler1D];
	if(v>=USampler1D && v<=USamplerBuffer) return samplerTable[v-USampler1D];

	static const ImageType imageTable[] = {
		ImageType_1D, ImageType_2D, ImageType_3D, ImageType_Cube, ImageType_1D, ImageType_1DArray,
		ImageType_2DArray, ImageType_CubeArray, ImageType_2D, ImageType_2DArray
	};
	if(v>=Image1D && v<=Image2DMSArray) return imageTable[v-Image1D];
	if(v>=IImage1D && v<=IImage2DMSArray) return imageTable[v-IImage1D];
	if(v>=UImage1D && v<=UImage2DMSArray) return imageTable[v-UImage1D];

	return ImageType_1D;
}

}}
