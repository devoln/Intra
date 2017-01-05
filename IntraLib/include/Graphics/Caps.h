#pragma once

#include "Platform/CppWarnings.h"
#include "Platform/Intrinsics.h"
#include "Core/FundamentalTypes.h"

namespace Intra { namespace Graphics {

INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS

struct DeviceCaps
{
	typedef uint Bool;

	DeviceCaps() {C::memset(this, 0, sizeof(*this));}

	uint MaxTexture2DSize: 16;
	uint MaxTexture3DSize: 16;
	uint MaxTextureArraySizeAndCount: 16;
	uint MaxColorAttachments: 4;
	uint MaxTextures: 7;
	uint MaxVertexTextures: 5;
	uint MaxFragmentTextures: 5;
	uint MaxAnisotropy: 5;

	//Форматы текстур
	Bool TextureRed: 1;
	Bool TextureRG: 1;
	Bool TextureBGRA: 1;

	Bool TextureDepth16: 1;
	Bool TextureDepth24: 1;
	Bool TextureDepth32: 1;
	Bool TextureDepth32F: 1;
	Bool TextureDepthStencil: 1;

	Bool TextureCubeDepth: 1;
	Bool DepthShadow: 1;
	
	Bool TextureInteger16: 1;
	Bool TextureInteger32: 1;

	Bool TextureFloat16: 1;
	Bool TextureFloat32: 1;
	Bool TextureFloat16Filtering: 1;
	Bool TextureFloat32Filtering: 1;
	Bool TextureFloat16Rendering: 1;
	Bool TextureFloat32Rendering: 1;
	Bool Index32Bit: 1;


	//Сжатые текстуры
	Bool TextureLATC: 1;
	Bool TextureRGTC: 1;
	Bool TextureBPTC: 1;
	Bool TextureDXT1: 1;
	Bool TextureDXT3: 1;
	Bool TextureDXT5: 1;
	Bool TextureETC1: 1;
	Bool TextureETC2: 1;

	Bool CopyBuffer: 1; //Прямое копирование буферов в видеопамяти
	Bool BufferTextureCopy: 1; //Прямое копирование между буферами и текстурами в видеопамяти
	Bool MapBuffer: 1; //В OpenGL ES 2.0 без расширений невозможно блокировать буфер, в Tegra 3+ есть нужное расширение, в ES 3.0 тоже появилось

	Bool HardwareIntegers: 1;
	Bool Texture3D: 1;
	Bool TextureArray: 1;
	Bool TextureSwizzle: 1;
	Bool HardwareInstancing: 1;
	Bool GeometryShader: 1;
	Bool TessellationShader: 1;
	Bool ShaderDynamicLinkage: 1;

	Bool DrawIndexedFirstVertex: 1;

	Bool IsDebugContext: 1;
	Bool LinearColorSpace: 1;

	uint FastCopyTexture: 2; //Прямое копирование данных текстуры с одинаковым форматом (>=1), с совместимым форматом (==2)
	uint ShaderFloatMantissaBits: 5, ShaderIntegerBits: 5;
};

INTRA_WARNING_POP

}}

