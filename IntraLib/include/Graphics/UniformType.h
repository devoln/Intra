#pragma once

#include "Core/Core.h"
#include "Containers/ForwardDeclarations.h"
#include "Data/ValueType.h"

namespace Intra {

enum ImageType: byte;

namespace Graphics {


struct UniformType: ValueType
{
	enum I: ushort
	{
		First=ValueType::End, FirstOfSamplers=First,
		Sampler1D=FirstOfSamplers, Sampler1DArray, Sampler2D, Sampler2DArray, SamplerCube, SamplerCubeArray, Sampler3D, Sampler2DMS, SamplerBuffer,
		Sampler2DShadow, Sampler2DArrayShadow, SamplerCubeShadow, SamplerCubeArrayShadow,
		ISampler1D, ISampler1DArray, ISampler2D, ISampler2DArray, ISamplerCube, ISamplerCubeArray, ISampler3D, ISampler2DMS, ISamplerBuffer,
		USampler1D, USampler1DArray, USampler2D, USampler2DArray, USamplerCube, USamplerCubeArray, USampler3D, USampler2DMS, USamplerBuffer,
		
		EndOfSamplers,

		FirstOfImages=EndOfSamplers,
		Image1D=FirstOfImages, Image2D, Image3D, ImageCube, ImageBuffer, Image1DArray, Image2DArray, ImageCubeArray, Image2DMS, Image2DMSArray,
		IImage1D, IImage2D, IImage3D, IImageCube, IImageBuffer, IImage1DArray, IImage2DArray, IImageCubeArray, IImage2DMS, IImage2DMSArray,
		UImage1D, UImage2D, UImage3D, UImageCube, UImageBuffer, UImage1DArray, UImage2DArray, UImageCubeArray, UImage2DMS, UImage2DMSArray,
		EndOfImages,
		
		UniformBlock=EndOfImages, StorageBlock,
		End
	};

	UniformType() = default;
	explicit UniformType(uint val): ValueType((ValueType::I)val) {}
	UniformType(I val): ValueType((ValueType::I)val) {}
	UniformType(ValueType val): ValueType(val) {}
	UniformType(ValueType::I val): ValueType(val) {}

	bool IsSampler() const {return (int)value>=FirstOfSamplers && (int)value<EndOfSamplers;}
	bool IsValid() const {return ValueType::IsValid() || (int)value>=(int)ValueType::End && (int)value<(int)UniformType::End;}

	ushort Size() const
	{
		if(ValueType::IsValid()) return ValueType::Size();
		return sizeof(int);
	}

	ImageType GetImageType() const;

	ValueType ToValueType() const;

	Intra::StringView ToStringGLSL() const;

	static UniformType FromStringGLSL(Intra::StringView str);
};

}}
