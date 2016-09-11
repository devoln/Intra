#pragma once

#include "Core/Core.h"
#include "Graphics/States.h"
#include "Math/Fixed.h"
#include "Algorithms/Hash.h"
#include "Graphics/UniformType.h"
#include "Imaging/ImagingTypes.h"
#include "Containers/String.h"
#include "Containers/IdAllocator.h"

namespace Intra { namespace Graphics {

struct SamplerDesc
{
	TexFilter MinFilter = TexFilter::Linear;
	TexFilter MagFilter = TexFilter::Linear;
	TexFilter MipFilter = TexFilter::Linear;

	TexWrap WrapS = TexWrap::Repeat;
	TexWrap WrapT = TexWrap::Repeat;
	TexWrap WrapR = TexWrap::Repeat;

	DepthTexCompareMode DepthCompareMode = DepthTexCompareMode::None;
	CompareFunc DepthCompareFunc = CompareFunc::Less;

	fixed8 MaxMip = 15;
	fixed8 MinMip = 0;
	sfixed8 MipBias = 0;

	bool operator==(const SamplerDesc& rhs) const {return core::memcmp(this, &rhs, sizeof(*this))==0;}
	bool operator!=(const SamplerDesc& rhs) const {return !operator==(rhs);}
};

struct SamplerCompactDesc
{
	SamplerCompactDesc() = default;
	SamplerCompactDesc(const SamplerCompactDesc&) = default;
	SamplerCompactDesc(const SamplerDesc& desc);

	bool operator==(const SamplerCompactDesc& rhs) const {return v1==rhs.v1 && v2==rhs.v2;}
	bool operator!=(const SamplerCompactDesc& rhs) const {return !operator==(rhs);}

	uint v1, v2;
};

struct ShaderInfo
{
	struct Uniform
	{
		UniformType Type;
		byte MatrixStride; //Старший бит - row major (1), column major (0). Остальные биты - расстояние в байтах между столбцами или строками
		ushort ArrayStride;
		uint ArraySize;
		uint LocOrOffset;
		String Name;
	};

	struct UniformBlock
	{
		String name;
		uint binding;
		Array<Uniform> uniforms;
	};

	struct Attribute
	{
		byte Location;
		ValueType Type;
		String Name;
	};

	Array<Uniform> DefaultBlock;
	VariableArray DefaultValues;
	Array<UniformBlock> UniformBlocks;
	Array<Attribute> Attributes;
};




struct MemoryBlockDesc
{
	size_t Size;
	MemoryFlags Flags;
};
typedef CheckedId<ushort, ushort, MemoryBlockDesc> MemoryBlockId;

struct TextureDesc
{
	ImageInfo Info;
	bool AutoGenMipmaps;
};
typedef CheckedId<ushort, ushort, TextureDesc> TextureId;

struct ShaderObjectDesc
{
	String code;
	ShaderType type;
	ShaderLang language;
};
typedef CheckedId<ushort, ushort, ShaderObjectDesc> ShaderObjectId;

struct AttribLocationBinding
{
	ushort id;
	String name;
};

struct ShaderProgramDesc
{
	ShaderObjectId Vertex, Fragment, Geometry, TessControl, TessEval;
	Array<AttribLocationBinding> AttribLocationBindings;
};
typedef CheckedId<ushort, ushort, ShaderProgramDesc> ShaderProgramId;

struct BufferViewDesc
{
	MemoryBlockId Block;
	uint StartInBlock, Size;
};
typedef CheckedId<ushort, ushort, BufferViewDesc> BufferViewId;

struct SyncDesc {};



struct AttributeStreamDesc
{
	struct Attribute {ushort Location; ValueType Type;};

	AttributeStreamDesc(BufferViewId buf=null, ArrayRange<const Attribute> attribs=null): Buffer(buf)
	{
		for(auto& da: Attributes) da = ValueType::Void;
		for(auto& attr: attribs) Attributes[attr.Location]=attr.Type;
	}

	BufferViewId Buffer;
	ValueType Attributes[16]; //На месте отсутствующего атрибута должно быть ValueType::Void
};

struct VertexAttribStateDesc
{
	Array<AttributeStreamDesc> Streams;
	BufferViewId IndexBuffer;
	uint FirstVertex;
	bool Indices32bit;
};
typedef CheckedId<ushort, ushort, VertexAttribStateDesc> VertexAttribStateId;


struct DrawCallDesc
{
	VertexAttribStateId VertexState;
	flag32 EnabledAttribs;
	uint FirstVertex, VertexCount;
	uint FirstIndex, IndexCount;
	uint Instances;
	bool Clockwise;
	PrimitiveTopology Topology;

	void SetDefault()
	{
		VertexState = null;
		EnabledAttribs = 0;
		FirstVertex = 0, VertexCount = 0;
		FirstIndex = 0, IndexCount = 0;
		Instances = 1;
		Clockwise = false;
		Topology = PrimitiveTopology::Triangles;
	}

	static const DrawCallDesc& Default()
	{
		static DrawCallDesc dcd;
		static bool inited=false;
		if(inited) return dcd;
		dcd.SetDefault();
		inited=true;
		return dcd;
	}
};

enum class AttachmentLoadOp: byte {Load, Clear, DontCare};
enum class AttachmentStoreOp: byte {Load, DontCare};

enum ImageLayout
{
	Undefined,
	General,
	ColorOptimal,
	DepthStencilOptimal,
	DepthStencilReadOnlyOptimal,
	ShaderReadOnlyOptimal,
	TransferSrcOptimal,
	TransferDstOptimal,
	Preinitialized
};

struct RenderPassDesc
{
	struct Subpass
	{
		struct AttachmentRef
		{
			uint Attachment;
			ImageLayout Layout;
		};

		struct Dependency
		{
			enum Flags: byte {ByRegion=1};

			uint SrcSubpass; //must be less or equal than DstSubpass
			uint DstSubpass;
			PipelineStageFlags SrcStageMask;
			PipelineStageFlags DstStageMask;
			AccessFlags SrcAccessMask;
			AccessFlags DstAccessMask;
			Flags DependencyFlags;
		};

		//! Which of the render pass’s attachments can be read in the shader during the subpass.
		//! InputAttahcments[i] => layout(input_attachment_index=i, ...).
		//! Input attachments must also be bound to the pipeline with a descriptor set
		Array<AttachmentRef> InputAttachments;

		//! ColorAttachments[i] => layout(location=X)
		Array<AttachmentRef> ColorAttachments;

		//! At the end of each subpass, color attachments are resolved to corresponding resolve attachments:
		//! if i<ResolveAttachments.Count and ResolveAttachments[i]!=UINT_MAX, then ColorAttachments[i] => ResolveAttachments[i]
		Array<AttachmentRef> ResolveAttachments;

		AttachmentRef DepthStencilAttachment;

		//! Indices describing the attachments that are not used by a subpass,
		//! but whose contents must be preserved throughout the subpass
		Array<uint> PreserveAttachments;
	};

	struct AttachmentDesc
	{
		ImageFormat Format;
		byte Samples;
		AttachmentLoadOp LoadOp;
		AttachmentStoreOp StoreOp;
		AttachmentLoadOp StencilLoadOp;
		AttachmentStoreOp StencilStoreOp;
		ImageLayout InitialLayout;
		ImageLayout FinalLayout;
	};

	Array<AttachmentDesc> Attachments;
	Array<Subpass> Subpasses;
	Array<Subpass::Dependency> Dependencies;
};
typedef CheckedId<ushort, ushort, RenderPassDesc> RenderPassId;

#if INTRA_DISABLED
//! Структура описывающая цель для рендеринга фреймбуфера
struct FramebufferAttachment
{
	TextureId Tex; //< В какую текстуру рендерить
	ushort Slice; //< В какой слой (3D текстуры, текстурного массива) или номер грани (для кубической текстуры) рендерить
	ushort Miplevel; //< Уровень mipmap текстуры, в который будет производиться рендеринг
};

struct FramebufferDesc
{
	Array<FramebufferAttachment> ColorAttachments;
	FramebufferAttachment DepthAttachment;
	FramebufferAttachment StencilAttachment;
};
#endif

struct FramebufferDesc
{
	RenderPassId RenderPass;
	Array<TextureId> Attachments;
	uint width, height, layers;
};




enum ShaderStageFlags: byte
{
	ShaderStageFlags_Vertex = 1, ShaderStageFlags_TessControl = 2, ShaderStageFlags_TessEval = 4,
	ShaderStageFlags_Geometry = 8, ShaderStageFlags_Fragment = 16, ShaderStageFlags_Compute = 32,
	ShaderStageFlags_AllGraphics = ShaderStageFlags_Vertex|ShaderStageFlags_TessControl|
	    ShaderStageFlags_TessEval|ShaderStageFlags_Geometry|ShaderStageFlags_Fragment,
	ShaderStageFlags_All = ShaderStageFlags_AllGraphics|ShaderStageFlags_Compute
};

enum class DescriptorType: byte
{
	Sampler,
	ImageSampler,
	SampledImage, //texture2D и т.п. в GLSL Vulkan
	StorageImage,
	UniformTexelBuffer, StorageTexelBuffer,
	UniformBuffer, StorageBuffer,
	UniformBufferDynamic, StorageBufferDynamic,
	InputAttachment
};


struct DescriptorSetLayoutDesc
{
	struct Binding
	{
		ushort BindingIndex;
		DescriptorType Type;
		ushort DescriptorCount;
		ShaderStageFlags StageFlags;
	};

	Array<Binding> Bindings;
};
typedef CheckedId<ushort, ushort, DescriptorSetLayoutDesc> DescriptorSetLayoutId;


enum DescriptorPoolFlags: uint {DescriptorPoolFlags_FreeDescriptorSet = 1};

struct DescriptorPoolDesc
{
	struct DescriptorPoolSize
	{
		DescriptorType Type;
		uint DescriptorCount;
	};

	DescriptorPoolFlags Flags;
	uint MaxSets;
	Array<DescriptorPoolSize> PoolSizes;
};
typedef CheckedId<ushort, ushort, DescriptorPoolDesc> DescriptorPoolId;



struct VertexInputStateDesc
{
	struct BindingDesc
	{
		byte Binding;
		bool PerInstance;
		ushort Stride;
	};

	struct AttributeDesc
	{
		byte Location;
		byte Binding;
		ValueType Type;
		uint Offset;
	};

	Array<BindingDesc> Bindings;
	Array<AttributeDesc> Attributes;
};

struct InputAssemblyDesc
{
	PrimitiveTopology Topology;
	bool UsePrimitiveRestartIndex;
};

struct TessellationStateDesc
{
	uint PatchControlPoints;
};

struct ViewportDesc
{
	Math::USVec2 Offset, Size;
	Math::USVec2 ScissorOffset, ScissorSize;
	Math::Vec2 DepthRange;
};

struct RasterizationStateDesc
{
	bool DepthClampEnable, RasterizerDiscard;
	PolyMode PolygonMode;
	CullMode CullingMode;
	Winding FrontFace;
	bool DepthBiasEnable;
	float DepthBiasConstantFactor, DepthBiasClamp, DepthBiasSlopeFactor;
	float LineWidth;

	static const RasterizationStateDesc Default;
};

struct MultisampleStateDesc
{
	byte RasterizationSamples; //Number of samples per pixel used in rasterization
	bool SampleShadingEnable; //Shading should occur per sample instead of per fragment
	float MinSampleShading; //Specifies the minimum number of unique sample locations that should be used during the given fragment’s shading
	
	//Contains a bitmask of static coverage information that is ANDed with the coverage information
	//generated during rasterization. Bits that are zero disable coverage for the corresponding sample.
	//Bit B of mask word M corresponds to sample 32×M+B32×M+B.
	//The array is sized to a length of [RasterizationSamples/32] words.
	//If SampleMask is null, it is treated as if the mask has all bits enabled, i.e. no coverage is removed from fragments.
	Array<uint> SampleMask;

	bool AlphaToCoverageEnable;
	bool AlphaToOneEnable; //each fragment alpha value is replaced by 1.0
};

struct StencilOpStateDesc
{
	StencilOp FailOp, PassOp, DepthFailOp;
	CompareFunc CompareOp;
	uint CompareMask; //bits of the unsigned integer stencil values participating in the stencil test
	uint WriteMask; //bits of the unsigned integer stencil values updated by the stencil test in the stencil framebuffer attachment
	uint Reference; //reference value that is used in the unsigned stencil comparison
};

struct DepthStencilStateDesc
{
	bool DepthTestEnable, DepthWriteEnable;
	CompareFunc DepthCompareOp;
	bool DepthBoundsTestEnable;

	bool StencilTestEnable;
	StencilOpStateDesc Front, Back;
	Math::Vec2 DepthBounds;
};

struct BlendAttachmentStateDesc
{
	bool BlendEnable;
	BlendFactor SrcColorBlendFactor, DstColorBlendFactor;
	BlendOperation ColorBlendOp;
	BlendFactor SrcAlphaBlendFactor, DstAlphaBlendFactor;
	BlendOperation AlphaBlendOp;
	ComponentMask ColorWriteMask;
};

struct BlendStateDesc
{
	bool LogicOpEnable;
	LogicOp LogicOperation;
	Array<BlendAttachmentStateDesc> Attachments;
	Math::Vec4 BlendConstantColor;
};

struct PipelineDynamicStateDesc
{
	Array<GraphicsPipelineState> DynamicStates;
};

struct PushConstantRange
{
	ShaderStageFlags StageFlags;
	uint Offset, Size;
};

struct PipelineLayoutDesc
{
	Array<DescriptorSetLayoutId> DescriptorSetLayouts;
	Array<PushConstantRange> PushConstantRanges;
};
typedef CheckedId<ushort, ushort, PipelineLayoutDesc> PipelineLayoutId;

struct GraphicsPipelineDesc;
typedef CheckedId<ushort, ushort, GraphicsPipelineDesc> GraphicsPipelineId;
struct GraphicsPipelineDesc
{
	ShaderProgramId ShaderProgram;
	VertexInputStateDesc VertexInputState;
	InputAssemblyDesc InputAssemblyState;
	TessellationStateDesc TessellationState;
	Array<ViewportDesc> Viewports;
	RasterizationStateDesc RasterizationState;
	MultisampleStateDesc MultisampleState;
	DepthStencilStateDesc DepthStencilState;
	BlendStateDesc ColorBlendState;
	PipelineDynamicStateDesc DynamicState;
	PipelineLayoutId Layout;
	RenderPassId RenderPass;
	uint Subpass;
	GraphicsPipelineId BasePipelineHandle;
};



typedef CheckedId<ushort, ushort, SamplerDesc> SamplerId;
typedef CheckedId<ushort, ushort, SyncDesc> SyncId;
typedef CheckedId<ushort, ushort, FramebufferDesc> FramebufferId;


}


inline uint ToHash(const Graphics::SamplerCompactDesc& s)
{
	return ToHash<uint>(s.v1)*(ToHash<uint>(s.v2)^0x87fa75df);
}

inline uint ToHash(const Graphics::SamplerDesc& desc)
{
	return ToHash(Graphics::SamplerCompactDesc(desc));
}

}
