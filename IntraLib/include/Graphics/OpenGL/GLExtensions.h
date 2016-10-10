#pragma once

#include "Containers/Array.h"
#include "Containers/String.h"

#if defined(_WIN32) && !defined(__SCITECH_SNAP__)
#   define KHRONOS_APICALL __declspec(dllimport)
#elif defined (__SYMBIAN32__)
#   define KHRONOS_APICALL IMPORT_C
#elif defined(__ANDROID__)
#   include <sys/cdefs.h>
#ifndef KHRONOS_APICALL
#define KHRONOS_APICALL /*__NDK_FPABI__*/
#endif
#else
#define KHRONOS_APICALL
#endif

#if !defined(OPENSTEP) && (defined(__WIN32__) && !defined(__CYGWIN__))
#if defined(__MINGW32__) && defined(GL_NO_STDCALL) || defined(UNDER_CE)
#define GLCALL
#else
#define GLCALL __stdcall
#endif
#elif defined(__CYGWIN__) && defined(USE_OPENGL32) /* use native windows opengl32 */
#define GLCALL __stdcall
#elif ((defined(__GNUC__) && __GNUC__ >= 4) || (defined(__SUNPRO_C) && (__SUNPRO_C >= 0x590))) && !defined(_MSC_VER)
#define GLCALL KHRONOS_APICALL
#endif /* WIN32 && !CYGWIN */


#define INTRA_EGL (INTRA_LIBRARY_WINDOW_SYSTEM==INTRA_LIBRARY_WINDOW_SYSTEM_AndroidNativeActivity || INTRA_LIBRARY_WINDOW_SYSTEM==INTRA_LIBRARY_WINDOW_SYSTEM_Emscripten)


#ifdef _MSC_VER
#define GLCALL __stdcall
#endif

namespace Intra {

typedef uint GLenum;
typedef unsigned char GLboolean;

struct OpenGL;

void InitExtensions(ushort version, bool noExtensions, OpenGL& gl);

//Проверка на поддержку wgl-расширения
bool IsWSGLExtensionSupported(StringView extension);

//Проверка на поддержку расширения OpenGL
bool IsExtensionSupported(StringView extension);

//Получить версию OpenGL
ushort GetGLVersion(bool* gles);


struct OpenGL
{

enum: ushort {
	BYTE=0x1400, UNSIGNED_BYTE=0x1401, SHORT=0x1402, UNSIGNED_SHORT=0x1403,
	INT=0x1404, UNSIGNED_INT=0x1405, FLOAT=0x1406, DOUBLE=0x140A
};

enum: ushort {NEVER=0x0200, LESS, EQUAL, LEQUAL, GREATER, NOTEQUAL, GEQUAL, ALWAYS};
enum: ushort {DEPTH_BUFFER_BIT=0x00000100, STENCIL_BUFFER_BIT=0x00000400, COLOR_BUFFER_BIT=0x00004000};

enum: ushort {
	POINTS= 0x0000, LINES=0x0001, LINE_LOOP= 0x0002, LINE_STRIP=0x0003,
	TRIANGLES=0x0004, TRIANGLE_STRIP=0x0005, TRIANGLE_FAN=0x0006
};

enum: ushort {
	ZERO=0, ONE=1, SRC_COLOR=0x0300, ONE_MINUS_SRC_COLOR=0x0301,
	SRC_ALPHA=0x0302, ONE_MINUS_SRC_ALPHA=0x0303, DST_ALPHA=0x0304, ONE_MINUS_DST_ALPHA=0x0305,

	DST_COLOR=0x0306, ONE_MINUS_DST_COLOR=0x0307, SRC_ALPHA_SATURATE=0x0308
};

enum: ushort {
	NONE=0, FRONT_LEFT=0x0400, FRONT_RIGHT=0x0401, BACK_LEFT=0x0402, BACK_RIGHT=0x0403,
	FRONT=0x0404, BACK=0x0405, LEFT=0x0406, RIGHT=0x0407, FRONT_AND_BACK=0x0408
};

#undef NO_ERROR
enum: ushort {
	NO_ERROR=0, INVALID_ENUM=0x0500, INVALID_VALUE=0x0501, INVALID_OPERATION=0x0502,
	STACK_OVERFLOW=0x0503, STACK_UNDERFLOW=0x0504, OUT_OF_MEMORY=0x0505
};

enum: ushort {CW=0x0900, CCW=0x0901};

enum: ushort {CULL_FACE=0x0B44, CULL_FACE_MODE=0x0B45, FRONT_FACE=0x0B46};

enum: ushort {
	DEPTH_RANGE=0x0B70, DEPTH_TEST=0x0B71, DEPTH_WRITEMASK=0x0B72,
	DEPTH_CLEAR_VALUE=0x0B73, DEPTH_FUNC=0x0B74,
	STENCIL_TEST=0x0B90, STENCIL_CLEAR_VALUE=0x0B91, STENCIL_FUNC=0x0B92,
	STENCIL_VALUE_MASK=0x0B93, STENCIL_FAIL=0x0B94,
	STENCIL_PASS_DEPTH_FAIL=0x0B95, STENCIL_PASS_DEPTH_PASS=0x0B96,
	STENCIL_REF=0x0B97, STENCIL_WRITEMASK=0x0B98,

	VIEWPORT=0x0BA2,
	BLEND_DST=0x0BE0, BLEND_SRC=0x0BE1, BLEND=0x0BE2,
	DRAW_BUFFER=0x0C01, READ_BUFFER=0x0C02, SCISSOR_BOX=0x0C10, SCISSOR_TEST=0x0C11,

	COLOR_CLEAR_VALUE=0x0C22, COLOR_WRITEMASK=0x0C23,

	UNPACK_SWAP_BYTES=0x0CF0, UNPACK_LSB_FIRST=0x0CF1, UNPACK_ROW_LENGTH=0x0CF2,
	UNPACK_SKIP_ROWS=0x0CF3, UNPACK_SKIP_PIXELS=0x0CF4, UNPACK_ALIGNMENT=0x0CF5,
	PACK_SWAP_BYTES=0x0D00, PACK_LSB_FIRST=0x0D01, PACK_ROW_LENGTH=0x0D02,
	PACK_SKIP_ROWS=0x0D03, PACK_SKIP_PIXELS=0x0D04, PACK_ALIGNMENT=0x0D05,

	TEXTURE_1D=0x0DE0, TEXTURE_2D=0x0DE1,

	TEXTURE_WIDTH=0x1000, TEXTURE_HEIGHT=0x1001, TEXTURE_INTERNAL_FORMAT=0x1003, TEXTURE_BORDER_COLOR=0x1004, TEXTURE_BORDER=0x1005,

	STENCIL_INDEX=0x1901, DEPTH_COMPONENT=0x1902,
	RED=0x1903, GREEN=0x1904, BLUE=0x1905, ALPHA=0x1906,
	RGB=0x1907, RGBA=0x1908, LUMINANCE=0x1909, LUMINANCE_ALPHA=0x190A,

	POINT=0x1B00, LINE=0x1B01, FILL=0x1B02,

	KEEP=0x1E00, REPLACE=0x1E01, INCR=0x1E02, DECR=0x1E03,

	VENDOR=0x1F00, RENDERER=0x1F01, VERSION=0x1F02, EXTENSIONS=0x1F03,

	NEAREST=0x2600, LINEAR=0x2601, NEAREST_MIPMAP_NEAREST=0x2700, LINEAR_MIPMAP_NEAREST=0x2701, NEAREST_MIPMAP_LINEAR=0x2702, LINEAR_MIPMAP_LINEAR=0x2703,
	TEXTURE_MAG_FILTER=0x2800, TEXTURE_MIN_FILTER=0x2801, TEXTURE_WRAP_S=0x2802, TEXTURE_WRAP_T=0x2803,
	CLAMP=0x2900, REPEAT=0x2901,

	ALPHA4=0x803B, ALPHA8=0x803C, ALPHA12=0x803D, ALPHA16=0x803E,
	LUMINANCE4=0x803F, LUMINANCE8=0x8040, LUMINANCE12=0x8041, LUMINANCE16=0x8042,
	LUMINANCE4_ALPHA4=0x8043, LUMINANCE6_ALPHA2=0x8044, LUMINANCE8_ALPHA8=0x8045, LUMINANCE12_ALPHA4=0x8046, LUMINANCE12_ALPHA12=0x8047, LUMINANCE16_ALPHA16=0x8048,
	INTENSITY=0x8049, INTENSITY4=0x804A, INTENSITY8=0x804B, INTENSITY12=0x804C, INTENSITY16=0x804D,
	R3_G3_B2=0x2A10, RGB4=0x804F, RGB5=0x8050, RGB8=0x8051, RGB10=0x8052, RGB12=0x8053, RGB16=0x8054,
	RGBA2=0x8055, RGBA4=0x8056, RGB5_A1=0x8057, RGBA8=0x8058, RGB10_A2=0x8059, RGBA12=0x805A, RGBA16=0x805B,

	BGR_EXT=0x80E0, BGRA_EXT=0x80E1,

	TEXTURE_RED_SIZE=0x805C, TEXTURE_GREEN_SIZE=0x805D, TEXTURE_BLUE_SIZE=0x805E, TEXTURE_ALPHA_SIZE=0x805F,
	TEXTURE_LUMINANCE_SIZE=0x8060, TEXTURE_INTENSITY_SIZE=0x8061,
	PROXY_TEXTURE_1D=0x8063, PROXY_TEXTURE_2D=0x8064,

	MAX_TEXTURE_SIZE=0x0D33
};

void(GLCALL *BindTexture)(GLenum target, uint texture);
void(GLCALL *BlendFunc)(GLenum sfactor, GLenum dfactor);
void(GLCALL *Clear)(flag32 mask);
void(GLCALL *ClearColor)(float red, float green, float blue, float alpha);
void(GLCALL *ClearDepth)(double depth);
void(GLCALL *ClearStencil)(int s);
const byte* (GLCALL *GetString)(GLenum name);
void(GLCALL *GenTextures)(int n, uint* textures);
void(GLCALL *GetBooleanv)(GLenum pname, GLboolean* params);
void(GLCALL *GetDoublev)(GLenum pname, double *params);
GLenum(GLCALL *GetError)(void);
void(GLCALL *GetFloatv)(GLenum pname, float* params);
void(GLCALL *GetIntegerv)(GLenum pname, int* params);
void(GLCALL *GetPointerv)(GLenum pname, void** params);

void(GLCALL *CopyTexImage1D)(GLenum target, int level, GLenum internalFormat, int x, int y, int width, int border);
void(GLCALL *CopyTexSubImage1D)(GLenum target, int level, int xoffset, int x, int y, int width);
void(GLCALL *GetTexImage)(GLenum target, int level, GLenum format, GLenum type, void* pixels);

void(GLCALL *CopyTexImage2D)(GLenum target, int level, GLenum internalFormat, int x, int y, int width, int height, int border);
void(GLCALL *CopyTexSubImage2D)(GLenum target, int level, int xoffset, int yoffset, int x, int y, int width, int height);
void(GLCALL *CullFace)(GLenum mode);
void(GLCALL *DeleteTextures)(int n, const uint *textures);
void(GLCALL *DepthFunc)(GLenum func);
void(GLCALL *DepthMask)(GLboolean flag);
void(GLCALL *DepthRange)(double zNear, double zFar);
void(GLCALL *Disable)(GLenum cap);
void(GLCALL *Enable)(GLenum cap);
void(GLCALL *DisableClientState)(GLenum array);
void(GLCALL *EnableClientState)(GLenum array);
void(GLCALL *DrawArrays)(GLenum mode, int first, int count);
void(GLCALL *DrawBuffer)(GLenum mode);
void(GLCALL *DrawElements)(GLenum mode, int count, GLenum type, const void* indices);
void(GLCALL *FrontFace)(GLenum mode);
void(GLCALL *GetTexParameterfv)(GLenum target, GLenum pname, float* params);
void(GLCALL *GetTexParameteriv)(GLenum target, GLenum pname, int* params);
GLboolean(GLCALL *IsEnabled)(GLenum cap);
GLboolean(GLCALL *IsTexture)(uint texture);
void(GLCALL *PolygonMode)(GLenum face, GLenum mode);
void(GLCALL *PolygonOffset)(float factor, float units);
void(GLCALL *ReadPixels)(int x, int y, int width, int height, GLenum format, GLenum type, void* pixels);
void(GLCALL *Scissor)(int x, int y, int width, int height);
void(GLCALL *StencilFunc)(GLenum func, int ref, uint mask);
void(GLCALL *StencilMask)(uint mask);
void(GLCALL *StencilOp)(GLenum fail, GLenum zfail, GLenum zpass);
void(GLCALL *TexImage1D)(GLenum target, int level, int internalformat, int width, int border, GLenum format, GLenum type, const void* pixels);
void(GLCALL *TexImage2D)(GLenum target, int level, int internalformat, int width, int height, int border, GLenum format, GLenum type, const void* pixels);
void(GLCALL *TexParameterf)(GLenum target, GLenum pname, float param);
void(GLCALL *TexParameterfv)(GLenum target, GLenum pname, const float *params);
void(GLCALL *TexParameteri)(GLenum target, GLenum pname, int param);
void(GLCALL *TexParameteriv)(GLenum target, GLenum pname, const int* params);
void(GLCALL *TexSubImage1D)(GLenum target, int level, int xoffset, int width, GLenum format, GLenum type, const void* pixels);
void(GLCALL *TexSubImage2D)(GLenum target, int level, int xoffset, int yoffset, int width, int height, GLenum format, GLenum type, const void* pixels);
void(GLCALL *Viewport)(int x, int y, int width, int height);
void(GLCALL *PixelStoref)(GLenum pname, float param);
void(GLCALL *PixelStorei)(GLenum pname, int param);
void(GLCALL *ColorMask)(GLboolean red, GLboolean green, GLboolean blue, GLboolean alpha);

decltype(Disable) DisEnable[2];


enum: ushort {MIRRORED_REPEAT=0x8370};

//Получение информации об ограничениях видеокарты
enum: ushort {
	MAX_FRAGMENT_UNIFORM_COMPONENTS=0x8B49, MAX_VERTEX_UNIFORM_COMPONENTS=0x8B4A,
	MAX_VARYING_FLOATS=0x8B4B, MAX_VERTEX_TEXTURE_IMAGE_UNITS=0x8B4C, MAX_COMBINED_TEXTURE_IMAGE_UNITS=0x8B4D
};

enum: ushort {INFO_LOG_LENGTH=0x8B84};

enum: ushort {
	ACTIVE_UNIFORMS=0x8B86, ACTIVE_UNIFORM_MAX_LENGTH=0x8B87,
	SHADER_SOURCE_LENGTH=0x8B88, ACTIVE_ATTRIBUTES=0x8B89, ACTIVE_ATTRIBUTE_MAX_LENGTH=0x8B8A
};

enum: ushort {
	VERTEX_SHADER_BIT=1, FRAGMENT_SHADER_BIT=2, GEOMETRY_SHADER_BIT=4,
	TESS_CONTROL_SHADER_BIT=8, TESS_EVALUATION_SHADER_BIT=16
};

enum: ushort {PROGRAM_SEPARABLE=0x8258};

//Шейдеры
enum: ushort {SHADING_LANGUAGE_VERSION=0x8B8C};

enum: ushort {
	VERTEX_SHADER=0x8B31, FRAGMENT_SHADER=0x8B30, GEOMETRY_SHADER=0x8DD9
};

enum: ushort {
	SHADER_TYPE=0x8B4F, COMPILE_STATUS=0x8B81, LINK_STATUS=0x8B82, VALIDATE_STATUS=0x8B83
};

enum: ushort {
	FLOAT_VEC2=0x8B50, FLOAT_VEC3=0x8B51, FLOAT_VEC4=0x8B52,
	DOUBLE_VEC2=0x8FFC, DOUBLE_VEC3=0x8FFD, DOUBLE_VEC4=0x8FFE,
	INT_VEC2=0x8B53, INT_VEC3=0x8B54, INT_VEC4=0x8B55,
	UNSIGNED_INT_VEC2=0x8DC6, UNSIGNED_INT_VEC3=0x8DC7, UNSIGNED_INT_VEC4=0x8DC8,
	BOOL=0x8B56, BOOL_VEC2=0x8B57, BOOL_VEC3=0x8B58, BOOL_VEC4=0x8B59
};


enum: ushort {
	FLOAT_MAT2=0x8B5A, FLOAT_MAT2x3=0x8B65, FLOAT_MAT2x4=0x8B66,
	FLOAT_MAT3=0x8B5B, FLOAT_MAT3x2=0x8B67, FLOAT_MAT3x4=0x8B68,
	FLOAT_MAT4=0x8B5C, FLOAT_MAT4x2=0x8B69, FLOAT_MAT4x3=0x8B6A,

	DOUBLE_MAT2=0x8F46, DOUBLE_MAT2x3=0x8F49, DOUBLE_MAT2x4=0x8F4A,
	DOUBLE_MAT3=0x8F47, DOUBLE_MAT3x2=0x8F4B, DOUBLE_MAT3x4=0x8F4C,
	DOUBLE_MAT4=0x8F48, DOUBLE_MAT4x2=0x8F4D, DOUBLE_MAT4x3=0x8F4E
};


enum: ushort {
	SAMPLER_1D=0x8B5D, SAMPLER_2D=0x8B5E, SAMPLER_3D=0x8B5F, SAMPLER_CUBE=0x8B60,
	SAMPLER_1D_SHADOW=0x8B61, SAMPLER_2D_SHADOW=0x8B62,
	SAMPLER_1D_ARRAY=0x8DC0, SAMPLER_2D_ARRAY=0x8DC1, SAMPLER_BUFFER=0x8DC2,
	SAMPLER_1D_ARRAY_SHADOW=0x8DC3, SAMPLER_2D_ARRAY_SHADOW=0x8DC4, SAMPLER_CUBE_SHADOW=0x8DC5,
	SAMPLER_CUBE_MAP_ARRAY=0x900C, SAMPLER_CUBE_MAP_ARRAY_SHADOW=0x900D, SAMPLER_2D_MULTISAMPLE=0x9108,
	
	INT_SAMPLER_1D=0x8DC9, INT_SAMPLER_2D=0x8DCA, INT_SAMPLER_3D=0x8DCB, INT_SAMPLER_CUBE=0x8DCC,
	INT_SAMPLER_1D_ARRAY=0x8DCE, INT_SAMPLER_2D_ARRAY=0x8DCF, INT_SAMPLER_BUFFER=0x8DD0,
	INT_SAMPLER_CUBE_MAP_ARRAY=0x900E, INT_SAMPLER_2D_MULTISAMPLE=0x9109, INT_SAMPLER_2D_MULTISAMPLE_ARRAY=0x910C,
	UNSIGNED_INT_SAMPLER_1D=0x8DD1, UNSIGNED_INT_SAMPLER_2D=0x8DD2, UNSIGNED_INT_SAMPLER_3D=0x8DD3,
	UNSIGNED_INT_SAMPLER_CUBE=0x8DD4, UNSIGNED_INT_SAMPLER_BUFFER=0x8DD8,
	UNSIGNED_INT_SAMPLER_2D_MULTISAMPLE=0x910A, UNSIGNED_INT_SAMPLER_2D_MULTISAMPLE_ARRAY=0x910D,
	
	UNSIGNED_INT_SAMPLER_1D_ARRAY=0x8DD6, UNSIGNED_INT_SAMPLER_2D_ARRAY=0x8DD7, UNSIGNED_INT_SAMPLER_CUBE_MAP_ARRAY=0x900F
};

uint(GLCALL *CreateShader)(GLenum target);
void(GLCALL *ShaderSource)(uint shader, int count, const char** String, const int* length);
void(GLCALL *CompileShader)(uint shader);
void(GLCALL *GetShaderiv)(uint shader, GLenum pname, int* params);
void(GLCALL *GetShaderInfoLog)(uint shader, int bufSize, int* length, char* infoLog);
void(GLCALL *DeleteShader)(uint shader);

void(GLCALL *UseProgram)(uint prog);
uint(GLCALL *CreateProgram)();
void(GLCALL *AttachShader)(uint prog, uint shader);
void(GLCALL *DetachShader)(uint prog, uint shader);
void(GLCALL *LinkProgram)(uint prog);
void(GLCALL *GetProgramiv)(uint prog, GLenum pname, int* params);
void(GLCALL *GetProgramInfoLog)(uint prog, int bufSize, int* length, char* infoLog);
void(GLCALL *DeleteProgram)(uint prog);

int(GLCALL *GetUniformLocation)(uint shader, const char* name);
void(GLCALL *GetActiveUniform)(uint prog, uint index, int maxLength, int* length, int* size, uint* type, char* name);
void(GLCALL *BindAttribLocation)(uint prog, uint index, const char* name);
void(GLCALL *GetActiveAttrib)(uint prog, uint index, int bufSize, int* length, int* size, GLenum* type, char* name);
int(GLCALL *GetAttribLocation)(uint prog, const char* name);
void(GLCALL *GetUniformfv)(uint prog, int loc, float* params);
void(GLCALL *GetUniformiv)(uint prog, int loc, int* params);
void(GLCALL *GetUniformuiv)(uint prog, int loc, float* params);
void(GLCALL *GetUniformdv)(uint prog, int loc, int* params);


#if(INTRA_MINEXE==0)
void(GLCALL *UseProgramStages)(uint pipeline, flag32 stages, uint prog);
void(GLCALL *GenProgramPipelines)(size_t n, uint *pipelines);
void(GLCALL *BindProgramPipeline)(uint pipeline);
void(GLCALL *DeleteProgramPipelines)(size_t n, const uint *pipelines);
#endif

void(GLCALL *ProgramParameteri)(uint prog, GLenum pname, int value);
void(GLCALL *GetProgramPipelineiv)(uint pipeline, GLenum pname, int *params);
void(GLCALL *ValidateProgramPipeline)(uint pipeline);
void(GLCALL *GetProgramPipelineInfoLog)(uint pipeline, size_t bufSize, size_t* length, char* infoLog);

#if(INTRA_MINEXE==0)
void(GLCALL *ProgramUniform1iv)(uint prog, int loc, size_t count, const int* value);
void(GLCALL *ProgramUniform2iv)(uint prog, int loc, size_t count, const int* value);
void(GLCALL *ProgramUniform3iv)(uint prog, int loc, size_t count, const int* value);
void(GLCALL *ProgramUniform4iv)(uint prog, int loc, size_t count, const int* value);

void(GLCALL *ProgramUniform1uiv)(uint prog, int loc, size_t count, const uint* value);
void(GLCALL *ProgramUniform2uiv)(uint prog, int loc, size_t count, const uint* value);
void(GLCALL *ProgramUniform3uiv)(uint prog, int loc, size_t count, const uint* value);
void(GLCALL *ProgramUniform4uiv)(uint prog, int loc, size_t count, const uint* value);

void(GLCALL *ProgramUniform1fv)(uint prog, int loc, size_t count, const float* value);
void(GLCALL *ProgramUniform2fv)(uint prog, int loc, size_t count, const float* value);
void(GLCALL *ProgramUniform3fv)(uint prog, int loc, size_t count, const float* value);
void(GLCALL *ProgramUniform4fv)(uint prog, int loc, size_t count, const float* value);

void(GLCALL *ProgramUniform1dv)(uint prog, int loc, size_t count, const double* value);
void(GLCALL *ProgramUniform2dv)(uint prog, int loc, size_t count, const double* value);
void(GLCALL *ProgramUniform3dv)(uint prog, int loc, size_t count, const double* value);
void(GLCALL *ProgramUniform4dv)(uint prog, int loc, size_t count, const double* value);

void(GLCALL *ProgramUniformMatrix2x3fv)(uint prog, int loc, size_t count, bool transpose, const float* value);
void(GLCALL *ProgramUniformMatrix3x2fv)(uint prog, int loc, size_t count, bool transpose, const float* value);
void(GLCALL *ProgramUniformMatrix2x4fv)(uint prog, int loc, size_t count, bool transpose, const float* value);
void(GLCALL *ProgramUniformMatrix4x2fv)(uint prog, int loc, size_t count, bool transpose, const float* value);
void(GLCALL *ProgramUniformMatrix3x4fv)(uint prog, int loc, size_t count, bool transpose, const float* value);
void(GLCALL *ProgramUniformMatrix4x3fv)(uint prog, int loc, size_t count, bool transpose, const float* value);
void(GLCALL *ProgramUniformMatrix2fv)(uint prog, int loc, size_t count, bool transpose, const float* value);
void(GLCALL *ProgramUniformMatrix3fv)(uint prog, int loc, size_t count, bool transpose, const float* value);
void(GLCALL *ProgramUniformMatrix4fv)(uint prog, int loc, size_t count, bool transpose, const float* value);

void(GLCALL *ProgramUniformMatrix2x3dv)(uint prog, int loc, size_t count, bool transpose, const double* value);
void(GLCALL *ProgramUniformMatrix3x2dv)(uint prog, int loc, size_t count, bool transpose, const double* value);
void(GLCALL *ProgramUniformMatrix2x4dv)(uint prog, int loc, size_t count, bool transpose, const double* value);
void(GLCALL *ProgramUniformMatrix4x2dv)(uint prog, int loc, size_t count, bool transpose, const double* value);
void(GLCALL *ProgramUniformMatrix3x4dv)(uint prog, int loc, size_t count, bool transpose, const double* value);
void(GLCALL *ProgramUniformMatrix4x3dv)(uint prog, int loc, size_t count, bool transpose, const double* value);
void(GLCALL *ProgramUniformMatrix2dv)(uint prog, int loc, size_t count, bool transpose, const double* value);
void(GLCALL *ProgramUniformMatrix3dv)(uint prog, int loc, size_t count, bool transpose, const double* value);
void(GLCALL *ProgramUniformMatrix4dv)(uint prog, int loc, size_t count, bool transpose, const double* value);
#endif

void(GLCALL *Uniform1iv)(int loc, size_t count, const int* value);
void(GLCALL *Uniform2iv)(int loc, size_t count, const int* value);
void(GLCALL *Uniform3iv)(int loc, size_t count, const int* value);
void(GLCALL *Uniform4iv)(int loc, size_t count, const int* value);

void(GLCALL *Uniform1uiv)(int loc, size_t count, const uint* value);
void(GLCALL *Uniform2uiv)(int loc, size_t count, const uint* value);
void(GLCALL *Uniform3uiv)(int loc, size_t count, const uint* value);
void(GLCALL *Uniform4uiv)(int loc, size_t count, const uint* value);

void(GLCALL *Uniform1fv)(int loc, size_t count, const float* value);
void(GLCALL *Uniform2fv)(int loc, size_t count, const float* value);
void(GLCALL *Uniform3fv)(int loc, size_t count, const float* value);
void(GLCALL *Uniform4fv)(int loc, size_t count, const float* value);

void(GLCALL *Uniform1dv)(int loc, size_t count, const double* value);
void(GLCALL *Uniform2dv)(int loc, size_t count, const double* value);
void(GLCALL *Uniform3dv)(int loc, size_t count, const double* value);
void(GLCALL *Uniform4dv)(int loc, size_t count, const double* value);

void(GLCALL *UniformMatrix2x3fv)(int loc, size_t count, bool transpose, const float* value);
void(GLCALL *UniformMatrix3x2fv)(int loc, size_t count, bool transpose, const float* value);
void(GLCALL *UniformMatrix2x4fv)(int loc, size_t count, bool transpose, const float* value);
void(GLCALL *UniformMatrix4x2fv)(int loc, size_t count, bool transpose, const float* value);
void(GLCALL *UniformMatrix3x4fv)(int loc, size_t count, bool transpose, const float* value);
void(GLCALL *UniformMatrix4x3fv)(int loc, size_t count, bool transpose, const float* value);
void(GLCALL *UniformMatrix2fv)(int loc, size_t count, bool transpose, const float* value);
void(GLCALL *UniformMatrix3fv)(int loc, size_t count, bool transpose, const float* value);
void(GLCALL *UniformMatrix4fv)(int loc, size_t count, bool transpose, const float* value);

void(GLCALL *UniformMatrix2x3dv)(int loc, size_t count, bool transpose, const double* value);
void(GLCALL *UniformMatrix3x2dv)(int loc, size_t count, bool transpose, const double* value);
void(GLCALL *UniformMatrix2x4dv)(int loc, size_t count, bool transpose, const double* value);
void(GLCALL *UniformMatrix4x2dv)(int loc, size_t count, bool transpose, const double* value);
void(GLCALL *UniformMatrix3x4dv)(int loc, size_t count, bool transpose, const double* value);
void(GLCALL *UniformMatrix4x3dv)(int loc, size_t count, bool transpose, const double* value);
void(GLCALL *UniformMatrix2dv)(int loc, size_t count, bool transpose, const double* value);
void(GLCALL *UniformMatrix3dv)(int loc, size_t count, bool transpose, const double* value);
void(GLCALL *UniformMatrix4dv)(int loc, size_t count, bool transpose, const double* value);

//Отладка

enum: ushort {
	DEBUG_OUTPUT_SYNCHRONOUS=0x8242, MAX_DEBUG_MESSAGE_LENGTH=0x9143, MAX_DEBUG_LOGGED_MESSAGES=0x9144,
    DEBUG_LOGGED_MESSAGES=0x9145, DEBUG_NEXT_LOGGED_MESSAGE_LENGTH=0x8243, DEBUG_CALLBACK_FUNCTION=0x8244,
    DEBUG_CALLBACK_USER_PARAM=0x8245
};

enum: ushort {
	DEBUG_SOURCE_API=0x8246, DEBUG_SOURCE_WINDOW_SYSTEM=0x8247, DEBUG_SOURCE_SHADER_COMPILER=0x8248,
	DEBUG_SOURCE_THIRD_PARTY=0x8249, DEBUG_SOURCE_APPLICATION=0x824A, DEBUG_SOURCE_OTHER=0x824B
};

enum: ushort {
	DEBUG_TYPE_ERROR=0x824C, DEBUG_TYPE_DEPRECATED_BEHAVIOR=0x824D, DEBUG_TYPE_UNDEFINED_BEHAVIOR=0x824E,
	DEBUG_TYPE_PORTABILITY=0x824F, DEBUG_TYPE_PERFORMANCE=0x8250, DEBUG_TYPE_OTHER=0x8251
};

enum: ushort {
	DEBUG_SEVERITY_HIGH=0x9146, DEBUG_SEVERITY_MEDIUM=0x9147, DEBUG_SEVERITY_LOW=0x9148
};

typedef void(GLCALL* DEBUGPROC)(GLenum source, GLenum type, uint id, GLenum severity, int length, const char* message, const void* userParam);
void(GLCALL *DebugMessageControl)(GLenum source, GLenum type, GLenum severity, uint count, const uint* ids, bool enabled);
void(GLCALL *DebugMessageInsert)(GLenum source, GLenum type, uint id, GLenum severity, uint length, const char* buf);
void(GLCALL *DebugMessageCallback)(DEBUGPROC callback, void* userParam);
uint(GLCALL *GetDebugMessageLog)(uint count, uint bufsize, GLenum* sources,
	GLenum* types, uint* ids, GLenum* severities, uint* lengths,  char* messageLog);




//Вершинные буферы
enum: ushort {
	ARRAY_BUFFER=0x8892, ELEMENT_ARRAY_BUFFER=0x8893,
	PIXEL_PACK_BUFFER=0x88EB, PIXEL_UNPACK_BUFFER=0x88EC,
	COPY_READ_BUFFER=0x8F36, COPY_WRITE_BUFFER=0x8F37
};

enum: ushort {
	STREAM_DRAW=0x88E0, STREAM_READ=0x88E1, STREAM_COPY=0x88E2,
	STATIC_DRAW=0x88E4, STATIC_READ=0x88E5, STATIC_COPY=0x88E6,
	DYNAMIC_DRAW=0x88E8, DYNAMIC_READ=0x88E9, DYNAMIC_COPY=0x88EA
};

enum: ushort {
	READ_ONLY=0x88B8, WRITE_ONLY=0x88B9, READ_WRITE=0x88BA
};

enum: ushort {
	MAP_READ_BIT=1, MAP_WRITE_BIT=2, MAP_INVALIDATE_RANGE_BIT=4,
	MAP_INVALIDATE_BUFFER_BIT=8, MAP_FLUSH_EXPLICIT_BIT=16, MAP_UNSYNCHRONIZED_BIT=32
};

void (GLCALL *GenBuffers)(uint n, const uint* buffers);
void (GLCALL *DeleteBuffers)(uint n, const uint* buffers);
void (GLCALL *BindBuffer)(GLenum target, uint buffer);
void (GLCALL *BufferData)(GLenum target, uint size, const void* data, GLenum usage);
void (GLCALL *BufferSubData)(GLenum target, uint offset, uint size, const void* data);

void (GLCALL *GetBufferSubData)(GLenum target, uint offset, uint size, void* data);

void (GLCALL *CopyBufferSubData)(GLenum readTarget, GLenum writeTarget, size_t readOffset, size_t writeOffset, size_t size);


enum: ushort {MAP_PERSISTENT_BIT=0x0040, MAP_COHERENT_BIT=0x0080};
void*(GLCALL *MapBuffer)(GLenum target, GLenum access);
void*(GLCALL *MapBufferRange)(GLenum target, intptr offset, intptr length, flag32 access);
void (GLCALL *FlushMappedBufferRange)(GLenum target, intptr offset, intptr length);
bool (GLCALL *UnmapBuffer)(GLenum target);

//sync
enum: ushort {
	MAX_SERVER_WAIT_TIMEOUT=0x9111, OBJECT_TYPE=0x9112, SYNC_CONDITION=0x9113, SYNC_STATUS=0x9114,
	SYNC_FLAGS=0x9115, SYNC_FENCE=0x9116, SYNC_GPU_COMMANDS_COMPLETE=0x9117, UNSIGNALED=0x9118, SIGNALED=0x9119,
	SYNC_FLUSH_COMMANDS_BIT=0x1, ALREADY_SIGNALED=0x911A, TIMEOUT_EXPIRED=0x911B, CONDITION_SATISFIED=0x911C, GL_WAIT_FAILED=0x911D
};

static const ulong64 TIMEOUT_IGNORED=0xFFFFFFFFFFFFFFFFull;

typedef struct {} *GLsync;
GLsync(GLCALL *FenceSync)(GLenum condition, uint flags);
GLboolean(GLCALL *IsSync)(GLsync sync);
void(GLCALL *DeleteSync)(GLsync sync);
GLenum(GLCALL *ClientWaitSync)(GLsync sync, uint flags, ulong64 timeout);
void(GLCALL *WaitSync)(GLsync sync, uint flags, ulong64 timeout);
void(GLCALL *GetInteger64v)(GLenum pname, long64 *params);
void(GLCALL *GetSynciv)(GLsync sync, GLenum pname, int bufSize, int* length, int* values);


//vertex arrays и gpu_shader4
void (GLCALL *VertexAttribPointer)(uint index, uint size, GLenum type, GLboolean normalized, int stride, const void* pointer);
void (GLCALL *DisableVertexAttribArray)(uint index);
void (GLCALL *EnableVertexAttribArray)(uint index);
void (GLCALL *DisEnableVertexAttribArray[2])(uint index);
void (GLCALL *VertexAttribIPointer)(uint index, int size, GLenum type, int stride, const void* pointer);

void(GLCALL *VertexAttribI1i)(uint index, int x);
void(GLCALL *VertexAttribI2i)(uint index, int x, int y);
void(GLCALL *VertexAttribI3i)(uint index, int x, int y, int z);
void(GLCALL *VertexAttribI4i)(uint index, int x, int y, int z, int w);

void(GLCALL *VertexAttribI1ui)(uint index, uint x);
void(GLCALL *VertexAttribI2ui)(uint index, uint x, uint y);
void(GLCALL *VertexAttribI3ui)(uint index, uint x, uint y, uint z);
void(GLCALL *VertexAttribI4ui)(uint index, uint x, uint y, uint z, uint w);

void(GLCALL *VertexAttribI1iv)(uint index, const int* v);
void(GLCALL *VertexAttribI2iv)(uint index, const int* v);
void(GLCALL *VertexAttribI3iv)(uint index, const int* v);
void(GLCALL *VertexAttribI4iv)(uint index, const int* v);

void(GLCALL *VertexAttribI1uiv)(uint index, const uint* v);
void(GLCALL *VertexAttribI2uiv)(uint index, const uint* v);
void(GLCALL *VertexAttribI3uiv)(uint index, const uint* v);
void(GLCALL *VertexAttribI4uiv)(uint index, const uint* v);

void(GLCALL *VertexAttribI4bv)(uint index, const byte* v);
void(GLCALL *VertexAttribI4sv)(uint index, const short* v);
void(GLCALL *VertexAttribI4ubv)(uint index, const byte* v);
void(GLCALL *VertexAttribI4usv)(uint index, const ushort* v);

void (GLCALL *GetVertexAttribIiv)(uint index, GLenum pname, int* params);
void (GLCALL *GetVertexAttribIuiv)(uint index, GLenum pname, uint* params);


int(GLCALL *GetFragDataLocation)(uint program, const char* name);



void (GLCALL *DrawArraysInstanced)(GLenum mode, int first, int count, int primcount);
void (GLCALL *DrawElementsInstanced)(GLenum mode, int count, GLenum type, const void* indices, int primcount);

void (GLCALL *DrawElementsBaseVertex)(GLenum mode, int count, GLenum type, const void* indices, const int basevertex);
void (GLCALL *DrawRangeElementsBaseVertex)(GLenum mode, uint start, uint end, int count, GLenum type, const void* indices, int basevertex);
void (GLCALL *DrawElementsInstancedBaseVertex)(GLenum mode, int count, GLenum type, const void* indices, int primcount, int basevertex);


enum: ushort {VERTEX_ATTRIB_ARRAY_DIVISOR=0x88FE};
void (GLCALL *VertexAttribDivisor)(uint index, uint divisor);


void(GLCALL *GenVertexArrays)(int n, uint* arrays);
void(GLCALL *BindVertexArray)(uint vao);
void(GLCALL *DeleteVertexArrays)(int n, const uint* arrays);



//UBO
enum: ushort {
	UNIFORM_BUFFER=0x8A11, UNIFORM_BUFFER_BINDING=0x8A28, UNIFORM_BUFFER_START=0x8A29, UNIFORM_BUFFER_SIZE=0x8A2A,
	MAX_VERTEX_UNIFORM_BLOCKS=0x8A2B, MAX_GEOMETRY_UNIFORM_BLOCKS=0x8A2C, MAX_FRAGMENT_UNIFORM_BLOCKS=0x8A2D, MAX_COMBINED_UNIFORM_BLOCKS=0x8A2E,
	MAX_UNIFORM_BUFFER_BINDINGS=0x8A2F, MAX_UNIFORM_BLOCK_SIZE=0x8A30,
	MAX_COMBINED_VERTEX_UNIFORM_COMPONENTS=0x8A31, MAX_COMBINED_GEOMETRY_UNIFORM_COMPONENTS=0x8A32, MAX_COMBINED_FRAGMENT_UNIFORM_COMPONENTS=0x8A33,
	UNIFORM_BUFFER_OFFSET_ALIGNMENT=0x8A34, ACTIVE_UNIFORM_BLOCK_MAX_NAME_LENGTH=0x8A35, ACTIVE_UNIFORM_BLOCKS=0x8A36,
	UNIFORM_TYPE=0x8A37, UNIFORM_SIZE=0x8A38, UNIFORM_NAME_LENGTH=0x8A39, UNIFORM_BLOCK_INDEX=0x8A3A, UNIFORM_OFFSET=0x8A3B,
	UNIFORM_ARRAY_STRIDE=0x8A3C, UNIFORM_MATRIX_STRIDE=0x8A3D, UNIFORM_IS_ROW_MAJOR=0x8A3E,
	UNIFORM_BLOCK_BINDING=0x8A3F, UNIFORM_BLOCK_DATA_SIZE=0x8A40, UNIFORM_BLOCK_NAME_LENGTH=0x8A41,
    UNIFORM_BLOCK_ACTIVE_UNIFORMS=0x8A42, UNIFORM_BLOCK_ACTIVE_UNIFORM_INDICES=0x8A43,
	UNIFORM_BLOCK_REFERENCED_BY_VERTEX_SHADER=0x8A44, UNIFORM_BLOCK_REFERENCED_BY_GEOMETRY_SHADER=0x8A45, UNIFORM_BLOCK_REFERENCED_BY_FRAGMENT_SHADER=0x8A46
};
static const uint INVALID_INDEX=0xFFFFFFFFu;


void(GLCALL *GetUniformIndices)(uint program, int uniformCount, const char* const * uniformNames, uint* uniformIndices);
void(GLCALL *GetActiveUniformsiv)(uint program, int uniformCount, const uint* uniformIndices, GLenum pname, int* params);
void(GLCALL *GetActiveUniformName)(uint program, uint uniformIndex, int bufSize, int* length, char* uniformName);
uint(GLCALL *GetUniformBlockIndex)(uint program, const char* uniformBlockName);
void(GLCALL *GetActiveUniformBlockiv)(uint program, uint uniformBlockIndex, GLenum pname, int* params);
void(GLCALL *GetActiveUniformBlockName)(uint program, uint uniformBlockIndex, int bufSize, int* length, char* uniformBlockName);
void(GLCALL *BindBufferRange)(GLenum target, uint index, uint buffer, intptr offset, intptr size);
void(GLCALL *BindBufferBase)(GLenum target, uint index, uint buffer);
void(GLCALL *GetIntegeri_v)(GLenum target, uint index, int* data);
void(GLCALL *UniformBlockBinding)(uint program, uint uniformBlockIndex, uint uniformBlockBinding);


//Текстуры
enum: ushort {
	TEXTURE0=0x84C0, MAX_TEXTURE_IMAGE_UNITS=0x8872
};

//Настройки sampler'а
enum: ushort {
	TEXTURE_MAX_ANISOTROPY=0x84FE, MAX_TEXTURE_MAX_ANISOTROPY=0x84FF,
	TEXTURE_WRAP_R=0x8072, CLAMP_TO_EDGE=0x812F, CLAMP_TO_BORDER=0x812D
};

enum: ushort {
	TEXTURE_SWIZZLE_R=0x8E42, TEXTURE_SWIZZLE_G=0x8E43,
	TEXTURE_SWIZZLE_B=0x8E44, TEXTURE_SWIZZLE_A=0x8E45, TEXTURE_SWIZZLE_RGBA=0x8E46
};

enum: ushort {
	TEXTURE_BASE_LEVEL=0x813C, TEXTURE_MAX_LEVEL=0x813D,
	TEXTURE_MIN_LOD=0x813A, TEXTURE_MAX_LOD=0x813B, TEXTURE_LOD_BIAS=0x8501,
	TEXTURE_COMPARE_MODE=0x884C, TEXTURE_COMPARE_FUNC=0x884D,
	COMPARE_R_TO_TEXTURE=0x884E
};

//Типы текстур
enum: ushort {
	PROXY_TEXTURE_3D=0x8070, PROXY_TEXTURE_1D_ARRAY=0x8C19, PROXY_TEXTURE_2D_ARRAY=0x8C1B,
	TEXTURE_DEPTH=0x8071, MAX_3D_TEXTURE_SIZE=0x8073, MAX_ARRAY_TEXTURE_LAYERS=0x88FF
};


#if(INTRA_PLATFORM_OS==INTRA_PLATFORM_OS_Android)
#undef UNSIGNED_SHORT_4_4_4_4
#undef UNSIGNED_SHORT_5_5_5_1
#undef UNSIGNED_SHORT_5_6_5
#endif

enum: ushort
{
	RED_INTEGER=0x8D94, GREEN_INTEGER, BLUE_INTEGER, ALPHA_INTEGER,
	RGB_INTEGER, RGBA_INTEGER, BGR_INTEGER, BGRA_INTEGER,

	DEPTH_STENCIL=0x84F9,
	DEPTH_COMPONENT16=0x81A5, DEPTH_COMPONENT24, DEPTH_COMPONENT32, DEPTH_COMPONENT32F=0x8CAC, DEPTH32F_STENCIL8, DEPTH24_STENCIL8=0x88F0,

	BGR=0x80E0, BGRA,
	RGBA32F=0x8814, RGB32F, LUMINANCE32F=0x8818, RGBA16F=0x881A, RGB16F, LUMINANCE16F=0x881E,

	SRGB=0x8C40, SRGB8, SRGB_ALPHA, SRGB8_ALPHA8, SLUMINANCE_ALPHA, SLUMINANCE8_ALPHA8, SLUMINANCE, SLUMINANCE8,
	COMPRESSED_SRGB, COMPRESSED_SRGB_ALPHA, COMPRESSED_SLUMINANCE, COMPRESSED_SLUMINANCE_ALPHA,
	COMPRESSED_SRGB_S3TC_DXT1_EXT, COMPRESSED_SRGB_ALPHA_S3TC_DXT1_EXT, COMPRESSED_SRGB_ALPHA_S3TC_DXT3_EXT, COMPRESSED_SRGB_ALPHA_S3TC_DXT5_EXT,

	COMPRESSED_RGB_S3TC_DXT1_EXT=0x83F0, COMPRESSED_RGBA_S3TC_DXT1_EXT, COMPRESSED_RGBA_S3TC_DXT3_EXT, COMPRESSED_RGBA_S3TC_DXT5_EXT,

	//RED_SNORM=0x8F90, RG_SNORM, RGB_SNORM, RGBA_SNORM,
	R8_SNORM=0x8F94, RG8_SNORM, RGB8_SNORM, RGBA8_SNORM, R16_SNORM, RG16_SNORM, RGB16_SNORM, RGBA16_SNORM,

	R8=0x8229, R16, RG8, RG16, R16F, R32F, RG16F, RG32F,
	R8I, R8UI, R16I, R16UI, R32I, R32UI,
	RG8I, RG8UI, RG16I, RG16UI, RG32I, RG32UI,
	COMPRESSED_RED=0x8225, COMPRESSED_RG, RG, RG_INTEGER=0x8228,

	RGBA32UI=0x8D70, RGB32UI, RGBA16UI=0x8D76, RGB16UI, RGBA8UI=0x8D7C, RGB8UI,
	RGBA32I=0x8D82, RGB32I, RGBA16I=0x8D88, RGB16I, RGBA8I=0x8D8E, RGB8I,

	RGB10_A2UI=0x906F, RGB9_E5=0x8C3D, R11F_G11F_B10F=0x8C3A, RGB565=0x8D62,

	COMPRESSED_R11_EAC=0x9270, COMPRESSED_SIGNED_R11_EAC, COMPRESSED_RG11_EAC, COMPRESSED_SIGNED_RG11_EAC,
	COMPRESSED_RGB8_ETC2, COMPRESSED_SRGB8_ETC2, COMPRESSED_RGB8_PUNCHTHROUGH_ALPHA1_ETC2,
	COMPRESSED_SRGB8_PUNCHTHROUGH_ALPHA1_ETC2, COMPRESSED_RGBA8_ETC2_EAC, COMPRESSED_SRGB8_ALPHA8_ETC2_EAC,

	COMPRESSED_RGBA_BPTC_UNORM=0x8E8C, COMPRESSED_SRGB_ALPHA_BPTC_UNORM, COMPRESSED_RGB_BPTC_SIGNED_FLOAT, COMPRESSED_RGB_BPTC_UNSIGNED_FLOAT,

	COMPRESSED_RED_RGTC1=0x8DBB, COMPRESSED_SIGNED_RED_RGTC1, COMPRESSED_RG_RGTC2, COMPRESSED_SIGNED_RG_RGTC2,
	_3DC_X_AMD=0x87F9, _3DC_XY_AMD, //Вроде тот же RGTC на Adreno (из расширения AMD_compressed_3DC_texture)

	COMPRESSED_LUMINANCE_LATC1_EXT=0x8C70, COMPRESSED_SIGNED_LUMINANCE_LATC1_EXT,
	COMPRESSED_LUMINANCE_ALPHA_LATC2_EXT, COMPRESSED_SIGNED_LUMINANCE_ALPHA_LATC2_EXT,

	COMPRESSED_RGB_PVRTC_4BPPV1_IMG=0x8C00, COMPRESSED_RGB_PVRTC_2BPPV1_IMG, COMPRESSED_RGBA_PVRTC_4BPPV1_IMG, COMPRESSED_RGBA_PVRTC_2BPPV1_IMG,

	COMPRESSED_LUMINANCE_ALPHA_3DC_ATI=0x8837,
	ATC_RGB_AMD=0x8C92, ATC_RGBA_EXPLICIT_ALPHA_AMD=0x8C93, ATC_RGBA_INTERPOLATED_ALPHA_AMD=0x87EE,

	ETC1_RGB8_OES=0x8D64,

	COMPRESSED_RGBA_ASTC_4x4_KHR=0x93B0, COMPRESSED_RGBA_ASTC_5x4_KHR, COMPRESSED_RGBA_ASTC_5x5_KHR,
	COMPRESSED_RGBA_ASTC_6x5_KHR, COMPRESSED_RGBA_ASTC_6x6_KHR, COMPRESSED_RGBA_ASTC_8x5_KHR,
	COMPRESSED_RGBA_ASTC_8x6_KHR, COMPRESSED_RGBA_ASTC_8x8_KHR, COMPRESSED_RGBA_ASTC_10x5_KHR,
	COMPRESSED_RGBA_ASTC_10x6_KHR, COMPRESSED_RGBA_ASTC_10x8_KHR, COMPRESSED_RGBA_ASTC_10x10_KHR,
	COMPRESSED_RGBA_ASTC_12x10_KHR, COMPRESSED_RGBA_ASTC_12x12_KHR,

	COMPRESSED_SRGB8_ALPHA8_ASTC_4x4_KHR=0x93D0, COMPRESSED_SRGB8_ALPHA8_ASTC_5x4_KHR, COMPRESSED_SRGB8_ALPHA8_ASTC_5x5_KHR,
	COMPRESSED_SRGB8_ALPHA8_ASTC_6x5_KHR, COMPRESSED_SRGB8_ALPHA8_ASTC_6x6_KHR, COMPRESSED_SRGB8_ALPHA8_ASTC_8x5_KHR,
	COMPRESSED_SRGB8_ALPHA8_ASTC_8x6_KHR, COMPRESSED_SRGB8_ALPHA8_ASTC_8x8_KHR, COMPRESSED_SRGB8_ALPHA8_ASTC_10x5_KHR,
	COMPRESSED_SRGB8_ALPHA8_ASTC_10x6_KHR, COMPRESSED_SRGB8_ALPHA8_ASTC_10x8_KHR, COMPRESSED_SRGB8_ALPHA8_ASTC_10x10_KHR,
	COMPRESSED_SRGB8_ALPHA8_ASTC_12x10_KHR, COMPRESSED_SRGB8_ALPHA8_ASTC_12x12_KHR,
};

enum: ushort
{
	UNSIGNED_BYTE_3_3_2=0x8032, UNSIGNED_SHORT_4_4_4_4, UNSIGNED_SHORT_5_5_5_1, UNSIGNED_INT_8_8_8_8, UNSIGNED_INT_10_10_10_2,
	UNSIGNED_SHORT_5_6_5=0x8363, UNSIGNED_INT_5_9_9_9_REV=0x8C3E, FLOAT_32_UNSIGNED_INT_24_8_REV=0x8DAD,
	UNSIGNED_INT_2_10_10_10_REV=0x8368, INT_2_10_10_10_REV=0x8D9F, UNSIGNED_INT_10F_11F_11F_REV=0x8C3B,
	UNSIGNED_SHORT_1_5_5_5_REV=0x8066,
	HALF_FLOAT=0x140B
};

enum: ushort { TEXTURE_CUBE_MAP=0x8513, TEXTURE_3D=0x806F, TEXTURE_1D_ARRAY=0x8C18, TEXTURE_2D_ARRAY=0x8C1A, TEXTURE_CUBE_MAP_ARRAY=0x9009 };
enum: ushort
{
	TEXTURE_CUBE_MAP_POSITIVE_X=0x8515, TEXTURE_CUBE_MAP_NEGATIVE_X, TEXTURE_CUBE_MAP_POSITIVE_Y,
	TEXTURE_CUBE_MAP_NEGATIVE_Y, TEXTURE_CUBE_MAP_POSITIVE_Z, TEXTURE_CUBE_MAP_NEGATIVE_Z
};


//#if(INTRA_PLATFORM_OS==INTRA_PLATFORM_OS_Windows)
//enum: ushort {R8=0x8229, RG8=0x822B, RG=0x8227, R16=0x822A;
//#endif

void(GLCALL *ActiveTexture)(GLenum texture);
void(GLCALL *TexImage3D)(GLenum target, int level, int internalFormat,
	int width, int height, int depth, int border, GLenum format, GLenum type, const void* data);
void(GLCALL *TexSubImage3D)(GLenum target, int level, int xoffset, int yoffset, int zoffset,
 	int width, int height, int depth, GLenum format, GLenum type, const void* data);
void (GLCALL *CopyTexSubImage3D)(GLenum target, int level,
		int xoffset, int yoffset, int zoffset, int x, int y, int width, int height);

void (GLCALL *CompressedTexImage2D)(GLenum target, int level, GLenum internalFormat, int width, int height, int border, int imageSize, const void* data);
void (GLCALL *CompressedTexSubImage2D)(GLenum target, int level, int xoffset, int yoffset, int width, int height, GLenum format, int imageSize, const void* data);
void (GLCALL *GetCompressedTexImage)(GLenum target, int lod, void* img); //Нет на GLES


//Рендеринг в текстуру
enum: ushort {INVALID_FRAMEBUFFER_OPERATION=0x0506};
enum: ushort {MAX_RENDERBUFFER_SIZE=0x84E8};
enum: ushort {FRAMEBUFFER_BINDING=0x8CA6, RENDERBUFFER_BINDING=0x8CA7};

enum: ushort {
	FRAMEBUFFER_ATTACHMENT_OBJECT_TYPE=0x8CD0, FRAMEBUFFER_ATTACHMENT_OBJECT_NAME=0x8CD1,
	FRAMEBUFFER_ATTACHMENT_TEXTURE_LEVEL=0x8CD2, FRAMEBUFFER_ATTACHMENT_TEXTURE_CUBE_MAP_FACE=0x8CD3,
	FRAMEBUFFER_ATTACHMENT_TEXTURE_3D_ZOFFSET=0x8CD4
};

enum: ushort {FRAMEBUFFER_COMPLETE=0x8CD5};

enum: ushort {
	FRAMEBUFFER_INCOMPLETE_ATTACHMENT=0x8CD6, FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT=0x8CD7,
	FRAMEBUFFER_INCOMPLETE_DUPLICATE_ATTACHMENT=0x8CD8, FRAMEBUFFER_INCOMPLETE_DIMENSIONS=0x8CD9,
	FRAMEBUFFER_INCOMPLETE_FORMATS=0x8CDA, FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER=0x8CDB, FRAMEBUFFER_INCOMPLETE_READ_BUFFER=0x8CDC
};

enum: ushort {FRAMEBUFFER_UNSUPPORTED=0x8CDD};

enum: ushort {
	MAX_COLOR_ATTACHMENTS=0x8CDF, COLOR_ATTACHMENT0=0x8CE0, COLOR_ATTACHMENT1=0x8CE1, COLOR_ATTACHMENT2=0x8CE2,
	COLOR_ATTACHMENT3=0x8CE3, DEPTH_ATTACHMENT=0x8D00, STENCIL_ATTACHMENT=0x8D20
};

enum: ushort {
	FRAMEBUFFER=0x8D40, DRAW_FRAMEBUFFER=0x8CA9, READ_FRAMEBUFFER=0x8CA8, RENDERBUFFER=0x8D41
};

enum: ushort {
	RENDERBUFFER_WIDTH=0x8D42, RENDERBUFFER_HEIGHT=0x8D43, RENDERBUFFER_INTERNAL_FORMAT=0x8D44
};

enum: ushort {
	STENCIL_INDEX1=0x8D46, STENCIL_INDEX4=0x8D47, STENCIL_INDEX8=0x8D48, STENCIL_INDEX16=0x8D49
};

enum: ushort {
	RENDERBUFFER_RED_SIZE=0x8D50, RENDERBUFFER_GREEN_SIZE=0x8D51, RENDERBUFFER_BLUE_SIZE=0x8D52,
	RENDERBUFFER_ALPHA_SIZE=0x8D53, RENDERBUFFER_DEPTH_SIZE=0x8D54, RENDERBUFFER_STENCIL_SIZE=0x8D55
};

enum: ushort {SAMPLER_BINDING=0x8919};

void(GLCALL *BindFramebuffer)(GLenum target, uint id);
void(GLCALL *GenFramebuffers)(int count, uint* ids);
void(GLCALL *DeleteFramebuffers)(int count, uint* ids);
bool(GLCALL *IsFramebuffer)(uint id);
void(GLCALL *GenRenderbuffers)(int count, uint* ids);
void(GLCALL *BindRenderbuffer)(GLenum target, uint renderbuffer);
void(GLCALL *DeleteRenderbuffers)(int count, uint* ids);
bool(GLCALL *IsRenderbuffer)(uint id);
void(GLCALL *RenderbufferStorage)(GLenum target, GLenum internalFormat, int width, int height);
void(GLCALL *FramebufferRenderbuffer)(GLenum target, GLenum attachment, GLenum rbTarget, uint rbId);
void(GLCALL *FramebufferTexture1D)(GLenum target, GLenum attachment, GLenum texTarget, uint texId, int level);
void(GLCALL *FramebufferTexture2D)(GLenum target, GLenum attachment, GLenum texTarget, uint texId, int level);
void(GLCALL *FramebufferTexture3D)(GLenum target, GLenum attachment, GLenum texTarget, uint texId, int level, int zOffset);
void(GLCALL *FramebufferTextureLayer)(GLenum target, GLenum attachment, uint texId, int level, int zOffset);
uint(GLCALL *CheckFramebufferStatus)(GLenum target);
void(GLCALL *GenerateMipmap)(GLenum target);

void(GLCALL *DrawBuffers)(int n, const GLenum* bufs);



void(GLCALL *CopyImageSubData)(uint srcName, GLenum srcTarget,
	int srcLevel, int srcX, int srcY, int srcZ,
	uint dstName, GLenum dstTarget, int dstLevel, int dstX, int dstY, int dstZ,
	int srcWidth, int srcHeight, int srcDepth);

//Sampler objects
void(GLCALL *GenSamplers)(uint count, uint* samplers);
void(GLCALL *DeleteSamplers)(uint count, const uint* samplers);
unsigned char(GLCALL *IsSampler)(uint sampler);
void(GLCALL *BindSampler)(uint unit, uint sampler);
void(GLCALL *SamplerParameteri)(uint sampler, GLenum pname, int param);
void(GLCALL *SamplerParameterf)(uint sampler, GLenum pname, float param);
void(GLCALL *SamplerParameteriv)(uint sampler, GLenum pname, const int* params);
void(GLCALL *SamplerParameterfv)(uint sampler, GLenum pname, const float* params);
void(GLCALL *GetSamplerParameteriv)(uint sampler, GLenum pname, int* params);
void(GLCALL *GetSamplerParameterfv)(uint sampler, GLenum pname, float* params);

//Multisample
enum: ushort {
	MULTISAMPLE=0x809D, SAMPLE_ALPHA_TO_COVERAGE=0x809E, SAMPLE_ALPHA_TO_ONE=0x809F, SAMPLE_COVERAGE=0x80A0
};

enum: ushort {
	SAMPLE_BUFFERS=0x80A8, SAMPLES=0x80A9, SAMPLE_COVERAGE_VALUE=0x80AA, SAMPLE_COVERAGE_INVERT=0x80AB, SAMPLE_MASK=0x8E51
};

void(GLCALL *SampleCoverage)(float value, GLboolean invert);

//Tessellation shader
void(GLCALL *PatchParameteri)(GLenum pname, int value);
void(GLCALL *PatchParameterfv)(GLenum pname, const float* values);

enum: ushort {
	PATCHES=0xE, PATCH_VERTICES=0x8E72, PATCH_DEFAULT_INNER_LEVEL=0x8E73, PATCH_DEFAULT_OUTER_LEVEL=0x8E74
};

enum: ushort {
	TESS_CONTROL_OUTPUT_VERTICES=0x8E75, TESS_GEN_MODE=0x8E76, TESS_GEN_SPACING=0x8E77,
	TESS_GEN_VERTEX_ORDER=0x8E78, TESS_GEN_POINT_MODE=0x8E79
};

enum: ushort {
	ISOLINES=0x8E7A, FRACTIONAL_ODD=0x8E7B, FRACTIONAL_EVEN=0x8E7C
};

enum: ushort {
	MAX_PATCH_VERTICES=0x8E7D, GLMAX_TESS_GEN_LEVEL=0x8E7E, MAX_TESS_CONTROL_UNIFORM_COMPONENTS=0x8E7F, MAX_TESS_EVALUATION_UNIFORM_COMPONENTS=0x8E80,
	MAX_TESS_CONTROL_TEXTURE_IMAGE_UNITS=0x8E81, MAX_TESS_EVALUATION_TEXTURE_IMAGE_UNITS=0x8E82, MAX_TESS_CONTROL_OUTPUT_COMPONENTS=0x8E83,
	MAX_TESS_PATCH_COMPONENTS=0x8E84, MAX_TESS_CONTROL_TOTAL_OUTPUT_COMPONENTS=0x8E85, MAX_TESS_EVALUATION_OUTPUT_COMPONENTS=0x8E86,
	MAX_TESS_CONTROL_UNIFORM_BLOCKS=0x8E89, MAX_TESS_EVALUATION_UNIFORM_BLOCKS=0x8E8A, MAX_TESS_CONTROL_INPUT_COMPONENTS=0x886C,
	MAX_TESS_EVALUATION_INPUT_COMPONENTS=0x886D, MAX_COMBINED_TESS_CONTROL_UNIFORM_COMPONENTS=0x8E1E,  MAX_COMBINED_TESS_EVALUATION_UNIFORM_COMPONENTS=0x8E1F
};

enum: ushort {
	UNIFORM_BLOCK_REFERENCED_BY_TESS_CONTROL_SHADER=0x84F0, UNIFORM_BLOCK_REFERENCED_BY_TESS_EVALUATION_SHADER=0x84F1
};

enum: ushort {
	TESS_EVALUATION_SHADER=0x8E87, TESS_CONTROL_SHADER=0x8E88
};


//Texture storage
void(GLCALL *TexStorage1D)(GLenum target, int levels, GLenum internalformat, int width);
void(GLCALL *TexStorage2D)(GLenum target, int levels, GLenum internalformat, int width, int height);
void(GLCALL *TexStorage3D)(GLenum target, int levels, GLenum internalformat, int width, int height, int depth);

enum: ushort {TEXTURE_IMMUTABLE_FORMAT=0x912F};


//Buffer storage
enum: ushort {DYNAMIC_STORAGE_BIT=0x0100, CLIENT_STORAGE_BIT=0x0200};
enum: ushort {BUFFER_IMMUTABLE_STORAGE=0x821F, BUFFER_STORAGE_FLAGS=0x8220};
enum: ushort {CLIENT_MAPPED_BUFFER_BARRIER_BIT=0x00004000};
void(GLCALL *BufferStorage)(GLenum target, intptr size, const void* data, uint flags);


//Multi Bind
void(GLCALL *BindBuffersBase)(GLenum target, uint first, int count, const uint* buffers);
void(GLCALL *BindBuffersRange)(GLenum target, uint first, int count, const uint* buffers, const intptr* offsets, const intptr* sizes);

void(GLCALL *BindTextures)(uint first, int count, const uint* textures);
void(GLCALL *BindSamplers)(uint first, int count, const uint* samplers);
void(GLCALL *BindImageTextures)(uint first, int count, const uint* textures);
void(GLCALL *BindVertexBuffers)(uint first, int count, const uint* buffers, const intptr* offsets, const int* strides);



//Другое
void(GLCALL *SwapInterval)(int interval);

enum: ushort {PROGRAM_POINT_SIZE=0x8642, POINT_SPRITE=0x8861};
enum: ushort {NUM_EXTENSIONS=0x821D};

enum: ushort {FRAMEBUFFER_SRGB=0x8db9};

//shader image load\store
enum: ushort {
	IMAGE_1D=0x904C, IMAGE_1D_ARRAY=0x9052, IMAGE_2D=0x904D, IMAGE_2D_RECT=0x904F,
	IMAGE_2D_ARRAY=0x9053, IMAGE_2D_MULTISAMPLE=0x9055, IMAGE_2D_MULTISAMPLE_ARRAY=0x9056, IMAGE_3D=0x904E,
	IMAGE_CUBE=0x9050, IMAGE_CUBE_MAP_ARRAY=0x9054, IMAGE_BUFFER=0x9051,

	INT_IMAGE_1D=0x9057, INT_IMAGE_1D_ARRAY=0x905D, INT_IMAGE_2D=0x9058, INT_IMAGE_2D_RECT=0x905A,
	INT_IMAGE_2D_ARRAY=0x905E, INT_IMAGE_2D_MULTISAMPLE=0x9060, INT_IMAGE_2D_MULTISAMPLE_ARRAY=0x9061,
	INT_IMAGE_3D=0x9059, INT_IMAGE_CUBE=0x905B, INT_IMAGE_CUBE_MAP_ARRAY=0x905F, INT_IMAGE_BUFFER=0x905C,

	UNSIGNED_INT_IMAGE_1D=0x9062, UNSIGNED_INT_IMAGE_1D_ARRAY=0x9068, UNSIGNED_INT_IMAGE_2D=0x9063, UNSIGNED_INT_IMAGE_2D_RECT=0x9065,
	UNSIGNED_INT_IMAGE_2D_ARRAY=0x9069, UNSIGNED_INT_IMAGE_2D_MULTISAMPLE=0x906B, UNSIGNED_INT_IMAGE_2D_MULTISAMPLE_ARRAY=0x906C,
	UNSIGNED_INT_IMAGE_3D=0x9064, UNSIGNED_INT_IMAGE_CUBE=0x9066, UNSIGNED_INT_IMAGE_CUBE_MAP_ARRAY=0x906A, UNSIGNED_INT_IMAGE_BUFFER=0x9067
};

enum: ushort {
	MAX_IMAGE_UNITS=0x8F38, MAX_COMBINED_IMAGE_UNITS_AND_FRAGMENT_OUTPUTS=0x8F39, MAX_IMAGE_SAMPLES=0x906D,
	MAX_VERTEX_IMAGE_UNIFORMS=0x90CA, MAX_TESS_CONTROL_IMAGE_UNIFORMS=0x90CB, MAX_TESS_EVALUATION_IMAGE_UNIFORMS=0x90CC,
	MAX_GEOMETRY_IMAGE_UNIFORMS=0x90CD, MAX_FRAGMENT_IMAGE_UNIFORMS=0x90CE, MAX_COMBINED_IMAGE_UNIFORMS=0x90CF
};

enum: ushort {
	IMAGE_BINDING_NAME=0x8F3A, IMAGE_BINDING_LEVEL=0x8F3B, IMAGE_BINDING_LAYERED=0x8F3C,
	IMAGE_BINDING_LAYER=0x8F3D, IMAGE_BINDING_ACCESS=0x8F3E, IMAGE_BINDING_FORMAT=0x906E
};

enum: uint {
	VERTEX_ATTRIB_ARRAY_BARRIER_BIT=0x00000001, ELEMENT_ARRAY_BARRIER_BIT=0x00000002, UNIFORM_BARRIER_BIT=0x00000004,
	TEXTURE_FETCH_BARRIER_BIT=0x00000008, SHADER_IMAGE_ACCESS_BARRIER_BIT=0x00000020, COMMAND_BARRIER_BIT=0x00000040,
	PIXEL_BUFFER_BARRIER_BIT=0x00000080, TEXTURE_UPDATE_BARRIER_BIT=0x00000100, BUFFER_UPDATE_BARRIER_BIT=0x00000200,
	FRAMEBUFFER_BARRIER_BIT=0x00000400, TRANSFORM_FEEDBACK_BARRIER_BIT=0x00000800, ATOMIC_COUNTER_BARRIER_BIT=0x00001000,
	ALL_BARRIER_BITS=0xFFFFFFFF
};

enum: ushort {
	IMAGE_FORMAT_COMPATIBILITY_TYPE=0x90C7, IMAGE_FORMAT_COMPATIBILITY_BY_SIZE=0x90C8, IMAGE_FORMAT_COMPATIBILITY_BY_CLASS=0x90C9
};

void(GLCALL *BindImageTexture)(uint unit, uint texture, int level, GLboolean layered, int layer, GLenum access, GLenum format);
void(GLCALL *MemoryBarrier)(uint barriers);

const byte*(GLCALL *GetStringi)(GLenum name, uint index);

enum: ushort {CLEAR_TEXTURE=0x9365};
void(GLCALL *ClearTexImage)(uint texture, int level, GLenum format, GLenum type, const void* data);
void(GLCALL *ClearTexSubImage)(uint texture, int level, int xoffset, int yoffset, int zoffset,
	int width, int height, int depth, GLenum format, GLenum type, const void* data);


////////////////////////
//Информация о контексте

String Extensions, VersionString, Vendor, Renderer;
ushort Version, GLSLVersion;
bool GLES;
bool IsDebugContext, IsCoreContext;

String ToString() const
{
	String result = VersionString+"\nВерсия GLSL: "+Intra::ToString(GLSLVersion)+'\n'+Renderer+'\n'+Vendor+'\n';
	result += "Контекст: ";
	if(IsCoreContext) result += "core";
	else result += "compatibility";
	if(IsDebugContext) result += " debug";
	result += "\n\nСписок поддерживаемых расширений:\n";
	for(auto&& ext: Extensions().Split(" "))
	{
		result+=ext;
		result+='\n';
	}
	return result;
}

struct Caps
{
	bool vertex_buffer_object, vertex_array_object, draw_elements_base_vertex;
	bool separate_shader_objects, gpu_shader4, copy_buffer, non_square_matrices, geometry_shader;
	bool gpu_shader5, shader_subroutine, tessellation_shader, copy_image;
	bool gpu_shader_fp64, shader_image_load_store, compute_shader, debug_output;
	bool texture_float, texture_half_float, texture_half_float_linear, texture_float_linear;
	bool color_buffer_half_float, color_buffer_float, texture_rg, texture_integer;
	bool texture_s3tc, texture_dxt1, texture_dxt3, texture_dxt5, texture_latc, texture_3dc, texture_rgtc, texture_bptc, texture_etc1, texture_etc2;
	bool framebuffer_object, texture_array, texture_cubemap_array, texture_3D, texture_bgra, texture_shared_exponent;
	bool texture_gather, transform_feedback, map_buffer, map_buffer_range, sync;
	bool texture_buffer_object, uniform_buffer_object, draw_instanced;
	bool instanced_arrays, texture_snorm, sampler_objects, texture_swizzle, texture_storage, buffer_storage;
	bool texture_depth16, texture_depth24, texture_depth32, texture_d24s8, texture_depth32f;
	bool depth_cube_map, texture_depth_comparison_modes, depth_copy_tex_image;
	bool point_sprite, texture_filter_anisotropic, element_index_uint;
	bool explicit_attrib_location, multi_bind;
	bool framebuffer_srgb, clear_texture;

	bool vsync, multisampling;
};

Caps Caps;

    OpenGL()
	{
		core::memset(this, 0, sizeof(OpenGL));
	}

	OpenGL& operator=(const OpenGL&) = delete;
};


#if(INTRA_LIBRARY_WINDOW_SYSTEM==INTRA_LIBRARY_WINDOW_SYSTEM_Windows)
enum: ushort {
	WGL_CONTEXT_FLAGS=0x2094, WGL_CONTEXT_DEBUG_BIT=1, WGL_CONTEXT_FORWARD_COMPATIBLE_BIT=2,
	WGL_CONTEXT_MAJOR_VERSION=0x2091, WGL_CONTEXT_MINOR_VERSION=0x2092, WGL_CONTEXT_LAYER_PLANE=0x2093,
	WGL_CONTEXT_PROFILE_MASK=0x9126, WGL_CONTEXT_CORE_PROFILE_BIT=1, WGL_CONTEXT_COMPATIBILITY_PROFILE_BIT=2,
	WGL_SAMPLE_BUFFERS_ARB=0x2041, WGL_SAMPLES_ARB=0x2042,
	WGL_NUMBER_PIXEL_FORMATS_ARB=0x2000, WGL_DRAW_TO_WINDOW_ARB=0x2001, WGL_DRAW_TO_BITMAP_ARB=0x2002, WGL_ACCELERATION_ARB=0x2003,
	WGL_SWAP_LAYER_BUFFERS_ARB=0x2006, WGL_SWAP_METHOD_ARB=0x2007, WGL_NUMBER_OVERLAYS_ARB=0x2008, WGL_NUMBER_UNDERLAYS_ARB=0x2009,
	WGL_TRANSPARENT_ARB=0x200A, WGL_TRANSPARENT_RED_VALUE_ARB=0x2037, WGL_TRANSPARENT_GREEN_VALUE_ARB=0x2038,
	WGL_TRANSPARENT_BLUE_VALUE_ARB=0x2039, WGL_TRANSPARENT_ALPHA_VALUE_ARB=0x203A, WGL_TRANSPARENT_INDEX_VALUE_ARB=0x203,
	WGL_SHARE_DEPTH_ARB=0x200C, WGL_SHARE_STENCIL_ARB=0x200D, WGL_SHARE_ACCUM_ARB=0x200E, WGL_SUPPORT_OPENGL_ARB=0x2010,
	WGL_DOUBLE_BUFFER_ARB=0x2011, WGL_STEREO_ARB=0x2012, WGL_PIXEL_TYPE_ARB=0x2013, WGL_COLOR_BITS_ARB=0x2014,
	WGL_RED_BITS_ARB=0x2015, WGL_RED_SHIFT_ARB=0x2016, WGL_GREEN_BITS_ARB=0x2017, WGL_GREEN_SHIFT_ARB=0x2018,
	WGL_BLUE_BITS_ARB=0x2019, WGL_BLUE_SHIFT_ARB=0x201A, WGL_ALPHA_BITS_ARB=0x201B, WGL_ALPHA_SHIFT_ARB=0x201C,
	WGL_DEPTH_BITS_ARB=0x2022, WGL_STENCIL_BITS_ARB=0x2023, WGL_SWAP_EXCHANGE_ARB=0x2028,
	WGL_SWAP_COPY_ARB=0x2029, WGL_SWAP_UNDEFINED_ARB=0x202A, WGL_TYPE_RGBA_ARB=0x202B
};
#endif

}