#if INTRA_LIBRARY_WINDOW_SYSTEM!=INTRA_LIBRARY_WINDOW_SYSTEM_Console

#include "Core/Core.h"
#include "GUI/MessageBox.h"
#include "Containers/StringView.h"
#include "Containers/String.h"
#include "Graphics/OpenGL/GLExtensions.h"

namespace Intra {

using namespace Math;

}

#if(INTRA_PLATFORM_OS==INTRA_PLATFORM_OS_Android)
#include <EGL/egl.h>
#include <GLES/gl.h>
#else
#if(INTRA_LIBRARY_WINDOW_SYSTEM==INTRA_LIBRARY_WINDOW_SYSTEM_Windows)
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <GL/gl.h>
#elif(INTRA_LIBRARY_WINDOW_SYSTEM==INTRA_LIBRARY_WINDOW_SYSTEM_X11)
#include <GL/glx.h>
#include <GL/gl.h>
#endif
#endif

#undef MemoryBarrier

#include "Platform/PlatformInfo.h"
#include "GUI/WindowSystemApi.h"

namespace Intra {

inline AnyPtr get_proc_address(const char* name)
{
#if(INTRA_PLATFORM_WINDOW_SYSTEM==INTRA_PLATFORM_WINDOW_SYSTEM_Windows)
	return (void*)wglGetProcAddress(name);
#elif(INTRA_PLATFORM_WINDOW_SYSTEM==INTRA_PLATFORM_WINDOW_SYSTEM_X11)
	return (void*)glXGetProcAddress((const byte*)name);
#elif(INTRA_PLATFORM_OS==INTRA_PLATFORM_OS_Android)
	return (void*)eglGetProcAddress(name);
#endif
}


//Проверка на поддержку расширения оконной системы
bool IsWSGLExtensionSupported(StringView extension)
{
#if(INTRA_PLATFORM_WINDOW_SYSTEM==INTRA_PLATFORM_WINDOW_SYSTEM_Windows)
	const auto wglGetExtensionsStringARB = (const char*(*)(HDC))get_proc_address("wglGetExtensionsStringARB");
	if(wglGetExtensionsStringARB==null) return false;
	const String allSupportedExtensions = " " + StringView(wglGetExtensionsStringARB(wglGetCurrentDC())) + " ";
#elif(INTRA_PLATFORM_WINDOW_SYSTEM==INTRA_PLATFORM_WINDOW_SYSTEM_X11)
	const auto glXGetExtensionsStringARB=(const char*(*)(HDC))get_proc_address("glXGetExtensionsStringARB");
	if(glXGetExtensionsStringARB==null) return false;
	const String allSupportedExtensions = " " + StringView(glXGetExtensionsStringARB()) + " ";
#elif defined(EGL)
	const String allSupportedExtensions = " " + StringView(eglQueryString(eglGetCurrentDisplay(), EGL_EXTENSIONS))+" ";
#endif
	return allSupportedExtensions().Contains(" " + extension + " ");
}

//Получение версии OpenGL
ushort GetGLVersion(bool* gles=null)
{
	StringView ver = StringView((const char*)glGetString(GL_VERSION));
	//В обычном OpenGL формат строки следующий: "<major>.<minor>[.<release_number>][ <vendor_specific>]"
    //В OpenGL ES формат строки другой: "OpenGL ES[-<profile>] <major>.<minor>[ <vendor_specific>]". Её парсить суть посложнее
	if(ver.Take(9)=="OpenGL ES")
	{
		if(gles) *gles=true;
		auto verPtr = ver.Data();
		verPtr += 9;
		if(*verPtr=='-') while(*verPtr!=' ' && *verPtr!='\0') verPtr++; //Пропускаем <profile>, если он присутсвует
		const bool firstIsDigit = verPtr[0]>='0' && verPtr[0]<='9';
		const bool secondIsDigit = verPtr[2]>='0' && verPtr[2]<='9';
		if(*verPtr=='\0' || !firstIsDigit || verPtr[1]!='.' || !secondIsDigit) return 20; //аналогично
	}
	if(gles) *gles=false;
	auto major = (ver[0]-'0');
	auto minor = (ver[2]-'0');
	return ushort( major*10 + minor );
}




struct Function
{
	void** func;
	const char* name;
};

static bool load_functions(ArrayRange<const Function> functions, ArrayRange<const char* const> suffices)
{
	for(ushort i=0; i<functions.Length(); i++)
	{
		for(auto suf: suffices)
		{
			String procName = "gl" + StringView(functions[i].name) + StringView(suf) + '\0';
			*functions[i].func = get_proc_address(procName.Data());
			if(*functions[i].func!=null) goto next_iteration;
		}
		return false;
	next_iteration:;
	}
	return true;
}



struct Extension
{
	byte minGL, minGLES;
	const char* exts;
	ArrayRange<const Function> functions;
	ArrayRange<const char* const> suffices;
};

static const char* const AllExtSuffices[]={"", "ARB", "OES", "EXT", "AMD", "NV", "ATI"};
static const char* const CrossVendorExtSuffices[]={"", "ARB", "OES", "EXT"};
static const char* const StdExtSuffices[]={"", "ARB", "OES"};
static const char* const CoreSuffix[]={""};

static bool LoadExtension(const Extension& ext, const OpenGL& gl)
{
	bool supported = gl.GLES? gl.Version>ext.minGL: gl.Version>ext.minGLES;

	if(!supported && ext.exts!=null)
	{
		for(auto extRequirements: StringView(ext.exts).Split("|"))
		{
			bool allSupported=true;
			for(auto extstr: extRequirements.Split("&"))
			{
				if(gl.Extensions().Contains("GL_"+extstr+" ")) continue;
				allSupported=false;
				break;
			}
			if(!allSupported) continue;
			supported=true;
			break;
		}
	}

	return supported && load_functions(ext.functions, ext.suffices);
}

#if(INTRA_LIBRARY_GRAPHICS==INTRA_LIBRARY_GRAPHICS_OpenGLES)
static void GLCALL clearDepth(double d) {glClearDepthf((float)d);}
static void GLCALL depthRange(double near, double far) {glDepthRangef((float)near, (float)far);}
static void GLCALL pixelStoref(GLenum pname, float param) {glPixelStorei(pname, (int)param);}
#endif

void InitExtensions(ushort version, bool noExtensions, OpenGL& gl)
{
	OpenGL& GL = gl;
	auto& caps = gl.Caps;
	gl.Vendor = (const char*)glGetString(GL_VENDOR);
	gl.Version = Min(GetGLVersion(&gl.GLES), version);

	auto glslstr = glGetString(GL.SHADING_LANGUAGE_VERSION);
	gl.GLSLVersion = ushort( 100*(glslstr[0]-'0')+10*(glslstr[2]-'0')+(glslstr[3]-'0') );
	gl.Renderer = (const char*)glGetString(GL_RENDERER);
	gl.VersionString = (const char*)glGetString(GL_VERSION);
	gl.IsCoreContext = false;

	bool allNeededExtensionsSupported=true;

#define I(f) gl.f = gl ## f
	I(Clear); I(ClearColor);

	I(ClearStencil);
	
	I(BindTexture); I(GenTextures); I(DeleteTextures); I(IsTexture); I(TexImage2D); I(TexSubImage2D);
	I(CopyTexImage2D); I(CopyTexSubImage2D);
	I(TexParameteri); I(TexParameteriv); I(TexParameterf); I(TexParameterfv);
	I(GetTexParameterfv); I(GetTexParameteriv);

	I(GetBooleanv); I(GetPointerv); I(GetFloatv); I(GetIntegerv); I(GetString);
	I(GetError);
	I(CullFace);
	I(DepthFunc); I(DepthMask);
	
	I(Disable); I(Enable); I(IsEnabled);
	gl.DisEnable[0]=gl.Disable, gl.DisEnable[1]=gl.Enable;
	I(DisableClientState); I(EnableClientState);

	I(DrawArrays); I(DrawElements);
	I(FrontFace);
	I(PolygonOffset);
	I(ReadPixels);
	I(Scissor);
	I(StencilFunc); I(StencilMask); I(StencilOp);
	I(Viewport);
	I(ColorMask);
	I(BlendFunc);
	I(PixelStorei);


#if(INTRA_LIBRARY_GRAPHICS!=INTRA_LIBRARY_GRAPHICS_OpenGLES)
	I(ClearDepth);
	I(CopyTexImage1D);
	I(CopyTexSubImage1D);
	I(GetTexImage);
	I(TexImage1D);
	I(TexSubImage1D);
	I(GetDoublev);
	I(DrawBuffer);
	I(DepthRange);
	I(PolygonMode);
	I(PixelStoref);
#else
	gl.ClearDepth=clearDepth;
	gl.DepthRange=depthRange;
	gl.PixelStoref=pixelStoref;
	I(CompressedTexImage2D);
	I(CompressedTexSubImage2D);
#endif


#undef I

	gl.GetStringi = get_proc_address("glGetStringi");
	if(!noExtensions)
	{
		if(gl.GetStringi==null)
		{
			GL.Extensions = StringView((const char*)gl.GetString(GL_EXTENSIONS));
			GL.Extensions += ' ';
		}
		else
		{
			int count=0;
			glGetIntegerv(GL.NUM_EXTENSIONS, &count);
			for(uint i=0; i<(uint)count; i++)
			{
				gl.Extensions += StringView((const char*)gl.GetStringi(GL_EXTENSIONS, i));
				gl.Extensions += ' ';
			}
		}
	}
	const char* const extensionsCStr = gl.Extensions.CStr();
//#define SUPPORTED(ext) caps.Extensions.Contains(ext " ")
#define SUPPORTED(ext) strstr(extensionsCStr, "GL_" ext " ")!=null
	caps.texture_depth_comparison_modes = LoadExtension({14, 30, "ARB_depth_texture", null, null}, gl);
	caps.point_sprite = LoadExtension({20, 10, "ARB_point_sprite", null, null}, gl);

	if(!gl.GLES && gl.Version>=14 || SUPPORTED("ARB_depth_texture"))
		caps.texture_depth16=caps.texture_depth24=caps.texture_depth32=true;
	else //В GLES glCopyTexImage с текстурами глубины не работает
	{
		caps.texture_depth16 = gl.Version>=30 || SUPPORTED("OES_depth_texture");
		caps.texture_depth24 = gl.Version>=30 || SUPPORTED("OES_depth24");
		caps.texture_depth32 = SUPPORTED("OES_depth32");
	}
	caps.texture_depth32f = gl.Version>=30 || SUPPORTED("ARB_depth_buffer_float");
	caps.depth_cube_map = LoadExtension({13, 30, "OES_depth_texture_cube_map", null, null}, gl);
	caps.depth_copy_tex_image = !gl.GLES;
	caps.texture_d24s8 = LoadExtension({31, 30, "EXT_packed_depth_stencil|OES_packed_depth_stencil", null, null}, gl);

if(!gl.GLES)
{
	caps.texture_float = caps.texture_float_linear = caps.texture_half_float =
		caps.texture_half_float_linear = gl.Version>=30 || SUPPORTED("ARB_texture_float");
	caps.color_buffer_float = caps.color_buffer_half_float = gl.Version>=30 || SUPPORTED("ARB_color_buffer_float");
}
else
{
	caps.texture_half_float = gl.Version>=30 || SUPPORTED("OES_texture_half_float");
	caps.texture_float = gl.Version>=30 || SUPPORTED("OES_texture_float");
	caps.texture_half_float_linear = gl.Version>=30 || SUPPORTED("OES_texture_half_float_linear");
	caps.texture_float_linear = SUPPORTED("OES_texture_float_linear");
	caps.color_buffer_half_float = caps.texture_half_float && SUPPORTED("EXT_color_buffer_half_float");
	caps.color_buffer_float = caps.texture_float && SUPPORTED("EXT_color_buffer_float");
}
	caps.texture_integer = gl.Version>=30 || SUPPORTED("EXT_texture_integer");

	caps.texture_s3tc = SUPPORTED("EXT_texture_compression_s3tc");
	caps.texture_dxt1 = SUPPORTED("EXT_texture_compression_dxt1");
	caps.texture_dxt3 = SUPPORTED("ANGLE_texture_compression_dxt3");
	caps.texture_dxt5 = SUPPORTED("ANGLE_texture_compression_dxt5");
	caps.texture_rgtc = LoadExtension({30, 255, "ARB_texture_compression_rgtc", null, null}, gl);
#ifndef MINIMIZE_GL
	caps.texture_latc = SUPPORTED("EXT_texture_compression_latc");
	caps.texture_3dc = SUPPORTED("AMD_compressed_3DC_texture");
	caps.texture_bptc = LoadExtension({42, 255, "ARB_texture_compression_bptc", null, null}, gl);
	caps.texture_etc1 = SUPPORTED("OES_compressed_ETC1_RGB8_texture");
	caps.texture_etc2 = LoadExtension({43, 30, "ARB_ES3_compatibility", null, null}, gl);
#endif

	caps.texture_rg = LoadExtension({30, 30, "ARB_texture_rg|EXT_texture_rg", null, null}, gl);
	caps.texture_bgra = LoadExtension({10, 255, "EXT_bgra|EXT_texture_format_BGRA8888", null, null}, gl);

	caps.texture_filter_anisotropic = SUPPORTED("EXT_texture_filter_anisotropic");
	caps.texture_swizzle = LoadExtension({33, 30, "ARB_texture_swizzle|EXT_texture_swizzle", null, null}, gl);

	caps.explicit_attrib_location = LoadExtension({33, 30, "ARB_explicit_attrib_location", null, null}, gl); //TODO: Насчёт версии GLES не уверен

#define F(func) {(void**)&gl.func, #func }
	//Шейдеры
	static const Function glslFunctions[]={
		F(CreateShader), F(ShaderSource), F(CompileShader),
		F(GetShaderiv), F(GetShaderInfoLog), F(DeleteShader),
		F(CreateProgram), F(AttachShader), F(DetachShader), F(LinkProgram), F(GetProgramiv),
		F(GetProgramInfoLog), F(UseProgram), F(DeleteProgram),
		F(GetUniformLocation), F(GetActiveUniform), F(BindAttribLocation), F(GetActiveAttrib), F(GetAttribLocation),
		F(GetUniformfv), F(GetUniformiv),

		F(Uniform1iv), F(Uniform2iv), F(Uniform3iv), F(Uniform4iv),
		F(Uniform1uiv), F(Uniform2uiv), F(Uniform3uiv), F(Uniform4uiv),
		F(Uniform1fv), F(Uniform2fv), F(Uniform3fv), F(Uniform4fv),
		F(UniformMatrix2fv), F(UniformMatrix3fv), F(UniformMatrix4fv)
	};

#ifndef MINIMIZE_GL
	caps.gpu_shader_fp64 = LoadExtension({40, 255, "ARB_gpu_shader_fp64",
	{
		F(Uniform1dv), F(Uniform2dv), F(Uniform3dv), F(Uniform4dv),
		F(UniformMatrix2dv), F(UniformMatrix3dv), F(UniformMatrix4dv), F(GetUniformdv)
	}, AllExtSuffices}, gl);
#endif

	caps.gpu_shader4 = LoadExtension({30, 30, "EXT_gpu_shader4",
	{
		F(Uniform1uiv), F(Uniform2uiv), F(Uniform3uiv), F(Uniform4uiv), F(GetUniformuiv),
		F(VertexAttribIPointer), F(GetVertexAttribIiv), F(GetVertexAttribIuiv), F(GetFragDataLocation),
		F(VertexAttribI1i), F(VertexAttribI2i), F(VertexAttribI3i), F(VertexAttribI4i),
		F(VertexAttribI1ui), F(VertexAttribI2ui), F(VertexAttribI3ui), F(VertexAttribI4ui),
		F(VertexAttribI1iv), F(VertexAttribI2iv), F(VertexAttribI3iv), F(VertexAttribI4iv),
		F(VertexAttribI1uiv), F(VertexAttribI2uiv), F(VertexAttribI3uiv), F(VertexAttribI4uiv),
		F(VertexAttribI4bv), F(VertexAttribI4sv), F(VertexAttribI4ubv), F(VertexAttribI4usv)
	}, AllExtSuffices}, gl);

	caps.texture_gather = LoadExtension({40, 31, "ARB_texture_gather", null, null}, gl);

#if(INTRA_MINEXE==0)
	caps.non_square_matrices = LoadExtension({30, 30, null,
	{
		F(UniformMatrix2x3fv), F(UniformMatrix2x4fv),
		F(UniformMatrix3x2fv), F(UniformMatrix3x4fv),
		F(UniformMatrix4x2fv), F(UniformMatrix4x3fv),
		F(UniformMatrix2dv), F(UniformMatrix3dv), F(UniformMatrix4dv),
		F(UniformMatrix2x3dv), F(UniformMatrix2x4dv),
		F(UniformMatrix3x2dv), F(UniformMatrix3x4dv),
		F(UniformMatrix4x2dv), F(UniformMatrix4x3dv)
	}, AllExtSuffices}, gl);
#endif

	if(!load_functions(glslFunctions, CoreSuffix)) allNeededExtensionsSupported=false;

	static const Function vertexArrayFunctions[]={
		F(VertexAttribPointer), F(DisableVertexAttribArray), F(EnableVertexAttribArray)
    };

	if(!load_functions(vertexArrayFunctions, AllExtSuffices)) allNeededExtensionsSupported=false;
	gl.DisEnableVertexAttribArray[false] = gl.DisableVertexAttribArray;
	gl.DisEnableVertexAttribArray[true] = gl.EnableVertexAttribArray;

	static const Function vboFunctions[]={
		F(GenBuffers), F(DeleteBuffers), F(BindBuffer),
		F(VertexAttribPointer), F(BufferData), F(BufferSubData),
#if(!EGL)
		F(GetBufferSubData),
#endif
		F(DisableVertexAttribArray), F(EnableVertexAttribArray)
	};
	caps.vertex_buffer_object = LoadExtension({15, 10, "ARB_vertex_buffer_object", vboFunctions, AllExtSuffices}, gl);
	if(!caps.vertex_buffer_object) allNeededExtensionsSupported=false;

//#ifndef INTRA_MINIMIZE_GL
	caps.vertex_array_object = LoadExtension({30, 30, "ARB_vertex_array_object",
	    {F(GenVertexArrays), F(BindVertexArray), F(DeleteVertexArrays)},
		AllExtSuffices}, gl);
//#endif

	caps.map_buffer = !gl.GLES && caps.vertex_buffer_object &&
		LoadExtension({15, 255, null, {F(MapBuffer), F(UnmapBuffer)}, AllExtSuffices}, gl);

	caps.map_buffer_range = LoadExtension({30, 30, "ARB_map_buffer_range|NV_map_buffer_range",
	    {F(MapBufferRange), F(UnmapBuffer), F(FlushMappedBufferRange)}, AllExtSuffices}, gl);

	caps.sync = LoadExtension({32, 30, "ARB_sync",
	{
		F(FenceSync), F(IsSync), F(DeleteSync), F(ClientWaitSync),
		F(WaitSync), F(GetInteger64v), F(GetSynciv)
	}, AllExtSuffices}, gl);

	caps.copy_buffer = LoadExtension({31, 30, "ARB_copy_buffer|EXT_copy_buffer|NV_copy_buffer",
		{F(CopyBufferSubData)}, AllExtSuffices}, gl);

	caps.uniform_buffer_object = LoadExtension({31, 30, "ARB_uniform_buffer_object|NV_uniform_buffer_object",
	{
		F(GetUniformIndices), F(GetActiveUniformsiv), F(GetActiveUniformName),
		F(GetUniformBlockIndex), F(GetActiveUniformBlockiv), F(GetActiveUniformBlockName),
		F(BindBufferRange), F(BindBufferBase), F(GetIntegeri_v), F(UniformBlockBinding)
	}, {"", "ARB", "NV"}}, gl);

	caps.transform_feedback = LoadExtension({30, 30, "EXT_transform_feedback", null, null}, gl);

	caps.framebuffer_srgb = (gl.Version>=30 || SUPPORTED("ARB_framebuffer_sRGB"));



	static const Function fboFunctions[] = {F(BindFramebuffer), F(GenFramebuffers), F(DeleteFramebuffers),
		F(IsFramebuffer), F(GenRenderbuffers), F(BindRenderbuffer), F(DeleteRenderbuffers), F(IsRenderbuffer),
		F(RenderbufferStorage), F(FramebufferRenderbuffer), F(CheckFramebufferStatus), F(FramebufferTexture2D),
		F(GenerateMipmap),  F(DrawBuffers)};
	caps.framebuffer_object = LoadExtension({30, 10, "ARB_draw_buffers&ARB_framebuffer_object|"
		"EXT_framebuffer_object&EXT_framebuffer_blit&EXT_framebuffer_multisample&EXT_packed_depth_stencil", fboFunctions, AllExtSuffices}, gl);
	if(!caps.framebuffer_object) allNeededExtensionsSupported=false;

	caps.draw_instanced = LoadExtension({31, 30, "ARB_draw_instanced|EXT_draw_instanced|NV_draw_instanced",
	    {F(DrawArraysInstanced), F(DrawElementsInstanced)}, {"", "ARB", "EXT", "NV"}}, gl);

	caps.instanced_arrays = LoadExtension({33, 30, "ARB_instanced_arrays|EXT_instanced_arrays|NV_instanced_arrays", {F(VertexAttribDivisor)}, {"", "ARB", "EXT", "NV"}}, gl);

//#ifndef MINIMIZE_GL
	//TODO: Ещё base vertex можно использовать в GLES 3.1 через glDrawElementsIndirect из VBO
	caps.draw_elements_base_vertex = LoadExtension({32, 32, "ARB_draw_elements_base_vertex|OES_draw_elements_base_vertex",
	    {F(DrawElementsBaseVertex), F(DrawRangeElementsBaseVertex), F(DrawElementsInstancedBaseVertex)}, StdExtSuffices}, gl);
//#endif

	load_functions({F(FramebufferTexture1D), F(FramebufferTexture3D), F(FramebufferTextureLayer)}, AllExtSuffices);

	caps.multisampling = LoadExtension({255, 10, "ARB_multisample", {F(SampleCoverage)}, AllExtSuffices}, gl);

#if(INTRA_PLATFORM_OS!=PLATFORM_OS_Windows)
if(gl.GLES)
#endif
	caps.texture_3D = LoadExtension({12, 30, "EXT_texture3D|OES_texture_3D",
	    {F(TexImage3D), F(TexSubImage3D), F(CopyTexSubImage3D)}, StdExtSuffices}, gl);
#if(INTRA_PLATFORM_OS!=INTRA_PLATFORM_OS_Windows)
else caps.texture_3D=true;
#endif

	caps.copy_image = LoadExtension({43, 32, "OES_copy_image|EXT_copy_image|ARB_copy_image|NV_copy_image",
		{F(CopyImageSubData)}, {"", "OES", "EXT", "ARB", "NV"}}, gl);
	caps.element_index_uint = LoadExtension({11, 30, "OES_element_index_uint", null, null}, gl);

    if(!gl.GLES) LoadExtension({13, 10, "ARB_texture_compression",
		{F(CompressedTexImage2D), F(GetCompressedTexImage), F(CompressedTexSubImage2D)}, {"", "ARB"}}, gl);

    caps.texture_array = LoadExtension({30, 30, "EXT_texture_array|ARB_texture_array", null, null}, gl);
	caps.texture_cubemap_array = LoadExtension({40, 255, "ARB_texture_cube_map_array|EXT_texture_cube_map_array", null, null}, gl);

	
#ifndef INTRA_MINIMIZE_GL

#ifndef INTRA_NO_ERROR_CHECKING
	caps.debug_output = LoadExtension({43, 255, "KHR_debug|AMD_debug_output|ARB_debug_output",
	    {F(DebugMessageControl), F(DebugMessageInsert), F(DebugMessageCallback), F(GetDebugMessageLog)}, {"", "KHR", "ARB", "AMD"}}, gl);
#endif

	static const Function vsyncFunction[]=
	{
#if(INTRA_LIBRARY_WINDOW_SYSTEM==INTRA_LIBRARY_WINDOW_SYSTEM_Windows)
	{(void**)&gl.SwapInterval, "wglSwapInterval"}
#elif(INTRA_LIBRARY_WINDOW_SYSTEM==INTRA_LIBRARY_WINDOW_SYSTEM_X11)
	{(void**)&gl.SwapInterval, "glXSwapInterval"}
#elif(INTRA_PLATFORM_OS==INTRA_PLATFORM_OS_Android)
	{(void**)&gl.SwapInterval, "eglSwapInterval"}
#else

#endif
	};

	caps.vsync = load_functions(vsyncFunction, {"", "ARB"});


	caps.shader_subroutine = LoadExtension({40, 255, "ARB_shader_subroutine", null, null}, gl);

	static const Function samplerObjFunctions[]={F(GenSamplers), F(DeleteSamplers), F(IsSampler), F(BindSampler),
		F(SamplerParameteri), F(SamplerParameterf), F(SamplerParameteriv), F(SamplerParameterfv), F(GetSamplerParameteriv), F(GetSamplerParameterfv)};
	caps.sampler_objects = LoadExtension({33, 30, "ARB_sampler_objects", samplerObjFunctions, StdExtSuffices}, gl);

	caps.tessellation_shader = LoadExtension({40, 32, "ARB_tessellation_shader|EXT_tessellation_shader",
	    {F(PatchParameteri), F(PatchParameterfv)}, AllExtSuffices}, gl);

	caps.texture_shared_exponent = LoadExtension({30, 30, "EXT_texture_shared_exponent", null, null}, gl);
	caps.texture_snorm = LoadExtension({31, 30, "EXT_texture_snorm", null, null}, gl);

	caps.texture_storage = (!gl.Vendor().Contains("AMD") && !gl.Vendor().Contains("ATI") || gl.Version>=40) && //На Radeon HD 4600 даже на самых новых драйверах обнаружены баги. Так мы их отсечём
		LoadExtension({42, 255, "ARB_texture_storage|EXT_texture_storage",
		    {F(TexStorage1D), F(TexStorage2D), F(TexStorage3D)}, {"", "ARB", "EXT"}}, gl);

	caps.buffer_storage = LoadExtension({44, 255, "ARB_buffer_storage", {F(BufferStorage)}, {"", "ARB"}}, gl);

	caps.geometry_shader = LoadExtension({32, 32, "ARB_geometry_shader4|EXT_geometry_shader4|EXT_geometry_shader", null, null}, gl);

	caps.shader_image_load_store = LoadExtension({42, 31, "ARB_shader_image_load_store|EXT_shader_image_load_store",
	    {F(BindImageTexture), F(MemoryBarrier)}, {"", "ARB", "EXT"}}, gl);

	caps.clear_texture = LoadExtension({44, 255, "ARB_clear_texture",
	    {F(ClearTexImage), F(ClearTexSubImage)}, {"", "ARB"}}, gl);

	//TODO загрузить все необходимые функции
	caps.gpu_shader5 = LoadExtension({40, 32, "ARB_gpu_shader5|EXT_gpu_shader5", null, null}, gl);
	caps.gpu_shader_fp64 = LoadExtension({40, 255, "ARB_gpu_shader_fp64", null, null}, gl);
	caps.compute_shader = LoadExtension({43, 31, "ARB_compute_shader", null, null}, gl);
	
	caps.texture_buffer_object = LoadExtension({31, 30, "ARB_texture_buffer_object|EXT_texture_buffer_object", null, null}, gl);
	caps.separate_shader_objects = LoadExtension({41, 255, "ARB_separate_shader_objects|EXT_separate_shader_objects", null, null}, gl);
	caps.multi_bind = LoadExtension({44, 255, "ARB_multi_bind", 
	{
		F(BindBuffersBase), F(BindBuffersRange), F(BindTextures),
		F(BindSamplers), F(BindImageTextures), F(BindVertexBuffers)
	}, StdExtSuffices}, gl);
#endif

	

	gl.IsDebugContext=false;

	//Другое
#if(INTRA_PLATFORM_OS!=INTRA_PLATFORM_OS_Linux)
	if(!load_functions({F(ActiveTexture)}, StdExtSuffices)) allNeededExtensionsSupported=false;
#endif

#undef F


#if(INTRA_MINEXE<=1)
	if(!allNeededExtensionsSupported)
	{
		String vendor = (const char*)glGetString(GL_VENDOR);
		vendor = vendor().ToUpperAscii();
		String message;
		if(vendor().Contains("NVIDIA"))
			message = "www.nvidia.ru/Download/index.aspx";
		else if(vendor().Contains("ATI") || vendor().Contains("AMD"))
			message = "http://support.amd.com/";
		else if(vendor().Contains("INTEL"))
			message = "downloadcenter.intel.com/";
		else
			message = "видеокарта не распознана";

		if(SUPPORTED("EXT_gpu_shader4") || SUPPORTED("ARB_geometry_shader4"))
			message = "Невозможно запустить приложение, так как драйвер видеокарты устарел.\nСайт для скачивания: "+message;
		else message = String::Format(
			"Для запуска программы требуется поддержка OpenGL 3.3! Текущая поддерживаемая версия - <^>.<^>. "
			"Попробуйте установить последние драйвера для вашей видеокарты.")(gl.Version/10)(gl.Version%10);

		ShowMessageBox(message, "Ошибка", MessageIcon::Error);
	}
#endif
}

}

#endif
