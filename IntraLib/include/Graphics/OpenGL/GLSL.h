#pragma once

#include "Containers/ForwardDeclarations.h"
#include "Graphics/States.h"

struct OpenGL;

namespace Intra { namespace GLSL {

//! Возвращает код, который нужно добавить в начало GLSL шейдера
//! типа type, чтобы сделать его максимально совместимым с контектом gl
String GetVersionCompatibilityCode(const OpenGL& gl, Graphics::ShaderType type);

//! Возвращает код, который нужно добавить в начало шейдера
//! типа type, чтобы добавить частичную поддержку HLSL в шейдер GLSL
String GetHlslCompatibilityCode(const OpenGL& gl, Graphics::ShaderType type);

}}

