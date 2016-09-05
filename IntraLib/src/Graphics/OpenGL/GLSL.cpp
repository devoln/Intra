#include "Graphics/OpenGL/GLSL.h"
#include "Containers/String.h"

namespace Intra { namespace GLSL {

#if INTRA_DISABLED
String GetVersionCompatibilityCode(const OpenGL& gl, Graphics::ShaderType type)
{
	return null;
}


String GetHlslCompatibilityCode(const OpenGL& gl, Graphics::ShaderType type)
{
	return null;
}
#endif


}}
