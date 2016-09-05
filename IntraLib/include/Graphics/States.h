#pragma once

#include "Data/Reflection.h"
#include "Data/Variable.h"
#include "Algorithms/Hash.h"


namespace Intra {

enum ImageType: byte;

namespace Graphics {

enum class LockAccess: byte {Read, Write, ReadWrite, Overwrite, NoLock};

enum class PrimitiveTopology: byte {Points, Lines, LineStrip, Triangles, TriangleStrip, TriangleFan, Quads, End};

enum class TexFilter: byte {
	Nearest, Linear, None,
	AnisotropicX2, AnisotropicX4, AnisotropicX8, AnisotropicX16,
	End
};
TexFilter TexFilter_FromString(StringView name);


enum class TexWrap: byte {Clamp, Repeat, MirroredRepeat, End};
TexWrap TexWrap_FromString(StringView str);

enum class CompareFunc: byte {
	LessEqual, GreaterEqual, Less, Greater,
	Equal, NotEqual, Always, Never,
	End
};
CompareFunc CompareFunc_FromString(StringView name);

enum class StencilOp: byte
{
	Keep, Zero, Replace, IncrementClamp,
	DecrementClamp, Invert, IncrementWrap, DecrementWrap
};

enum class DepthTexCompareMode: byte {None, CompareRtoTexture, End};
DepthTexCompareMode DepthTexCompareMode_FromString(StringView str);

//! Параметры, которые используются для выбора желаемого устройства для выделения в нём памяти
typedef flag8 MemoryFlags;
enum: MemoryFlags
{
	MemoryFlags_None=0,

	//! Выделить в памяти устройства (видеопамяти)
	MemoryFlags_DeviceLocal=1,

	//! Требуется возможность отображения буфера в память для записи
	MemoryFlags_HostWritable=2,

	//! ... для чтения
	MemoryFlags_HostReadable=4,

	//! ... для чтения и записи
	MemoryFlags_HostReadWrite = MemoryFlags_HostWritable|MemoryFlags_HostReadable,

	//! Изменения со стороны устройства или хоста будут становиться видимы сразу без команды flush.
	//! Требует HostWriteable или HostReadable
	MemoryFlags_HostCoherent=8,

	//! Память кешируется на хосте. Это быстрее, чем без кеширования, но не всегда coherent.
	//! Требует (HostWriteable или HostReadable) и HostPersistent
	MemoryFlags_HostCached=16,

	//! Устройство может использовать mapped память. Требует HostWriteable или HostReadable
	MemoryFlags_HostPersistent=32,

	//! Память вероятно будет использоваться устройством всего небольшое количество раз между обновлениями со стороны хоста (хинт)
	MemoryFlags_HostStreamed=64
};

typedef flag8 ClearFlags;
enum: ClearFlags {ClearFlags_Color=1, ClearFlags_Depth=2, ClearFlags_Stencil=4};



//Числа совпадают с Vulkan
enum class LogicOp: byte
{
	Clear, And, AndReverse, Copy, AndInverted,
	NoOp, Xor, Or, NotOr, Equivalent, Invert,
	OrReverse, CopyInverted, OrInverted, NotAnd, Set
};


enum class BlendFactor: byte
{
	Zero, One,
	Src_Color, InvSrc_Color, Dst_Color, InvDst_Color,
	Src_Alpha, InvSrc_Alpha, Dst_Alpha, InvDst_Alpha,
	ConstantColor, InvConstantColor, ConstantAlpha, InvConstantAlpha,
	SrcAlphaSaturate, Src1Color, InvSrc1Color, Src1Alpha, InvSrc1Alpha,
	End
};

BlendFactor BlendFactor_FromString(StringView name);

enum class BlendOperation: byte {Add, Subtract, ReverseSubtract};

typedef flag8 ComponentMask;
enum: byte {
	ComponentMask_X=1, ComponentMask_R=ComponentMask_X,
	ComponentMask_Y=2, ComponentMask_G=ComponentMask_Y,
	ComponentMask_Z=4, ComponentMask_B=ComponentMask_Z,
	ComponentMask_W=8, ComponentMask_A=ComponentMask_W,
	ComponentMask_XYZ = ComponentMask_X|ComponentMask_Y|ComponentMask_Z, ComponentMask_RGB=ComponentMask_XYZ,
	ComponentMask_XYZW = ComponentMask_XYZ|ComponentMask_W, ComponentMask_RGBA=ComponentMask_XYZW
};

enum class PolyMode: ushort {Fill, Line, Point, End};
PolyMode PolyMode_FromString(StringView name);


enum class CullMode {None, Front, Back, FrontAndBack, End};

enum class Winding {CCW, CW};

enum class GraphicsPipelineState
{
	Viewport, Scissor, LineWidth, DepthBias, BlendConstants,
	DepthBounds, StencilCompareMask, StencilWriteMask, StencilReference
};

enum class ShaderType: ushort {Vertex, Fragment, Geometry, TessControl, TessEval, Compute, Count, Null};
ShaderType ShaderType_FromString(StringView typeName);

enum class PipelineStageFlags: uint
{
	TopOfPipe = 1, //< Commands are initially received by the queue
	DrawIndirect = 2, //< Draw/DispatchIndirect data structures are consumed
	VertexInput = 4, //< Vertex and index buffers are consumed
	VertexShader = 8,
	TessControlShader = 0x10,
	TessEvalShader = 0x20,
	GeometryShader = 0x40,
	FragmentShader = 0x80,
	EarlyFragmentTests = 0x100, //< Early fragment tests (depth and stencil tests before fragment shading) are performed.
	LateFragmentTests = 0x200, //< Late fragment tests (depth and stencil tests after fragment shading) are performed.
	ColorAttachmentOutput = 0x400, //< After blending where the final color values are output from the pipeline + resolve operations.
	ComputeShader = 0x800,
	Transfer = 0x1000, //< The operations resulting from all transfer commands.
	BottomOfPipe = 0x2000, //< Final stage in the pipeline where commands complete execution.
	Host = 0x4000, //< A pseudo-stage indicating execution on the host of reads/writes of device memory.
	AllGraphics = 0x8000, //< Execution of all graphics pipeline stages.
	AllCommands = 0x10000 //< Execution of all stages supported on the queue.
};

enum AccessFlags
{
	IndirectCommandRead = 1, //< indirect command structure read as part of an indirect drawing command
	IndexRead = 2, //< index buffer read
	VertexAttributeRead = 4, //< read via the vertex input bindings
	UniformRead = 8, //< read via a uniform buffer or dynamic uniform buffer descriptor
	InputAttachmentRead = 0x10, //< read via an input attachment descriptor
	ShaderRead = 0x20, //< read from a shader via any other descriptor type.
	ShaderWrite = 0x40, //< write or atomic from a shader via the same descriptor types as in VK_ACCESS_SHADER_READ_BIT
	ColorAttachmentRead = 0x80, //< read via a color attachment
	ColorAttachmentWrite = 0x100, //< write via a color attachment
	DepthStencilAttachmentRead = 0x200, //< read via a depth/stencil attachment
	DepthStencilAttachmentWrite = 0x400, //< write via a depth/stencil attachment
	TransferRead = 0x800, //< read from a transfer (copy, blit, resolve, etc.) operation
	TransferWrite = 0x1000, //< write from a transfer (copy, blit, resolve, etc.) operation
	HostRead = 0x2000, //< read via the host.
	HostWrite = 0x4000, //< write via the host.
	MemoryRead = 0x8000, //< read via a non-specific unit attached to the memory
	MemoryWrite = 0x10000 //< write via a non-specific unit attached to the memory
};

enum class GraphicsAPI: byte {OpenGL, Direct3D9, Direct3D10, Direct3D11, Direct3D12, Vulkan, Mantle, Metal};

enum class DrawCategory: byte {Invisible, Auto, First, PreOpaque, Opaque, PostOpaque, OrderDependent, PostOrderDependent, Last};



enum class ShaderLang: byte {GLSL, HLSL9, HLSL11, Cg, GL_Asm, D3D_Asm, SPIR_V};

enum class GpuVendor: byte {Unknown, NVidia, AMD, Intel, Imagination, Qualcomm, ARM, Mesa};

struct FatMeshVertex
{
	Math::Vec3 aPosition;
	Math::Vec2 aTexCoord;
	Math::Vec3 aNormal, aTangent, aBinormal;

	Math::Vec4 aBoneWeights;
	Math::UBVec4 aBoneIndices;

	INTRA_ADD_REFLECTION(FatMeshVertex, aPosition, aTexCoord, aNormal, aTangent, aBinormal, aBoneWeights, aBoneIndices);
};

struct DrawCall;


}}
