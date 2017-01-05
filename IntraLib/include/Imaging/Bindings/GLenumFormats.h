#pragma once

#include "Core/FundamentalTypes.h"

namespace Intra { namespace Imaging {

struct ImageFormat;
enum ImageType: byte;
enum class CubeFace: byte;

ushort ImageFormatToGLInternal(ImageFormat format, bool useSwizzling);
ushort ImageFormatToGLExternal(ImageFormat format, bool swapRB, bool useSwizzling);
ushort ImageFormatToGLType(ImageFormat format);
bool GLFormatSwapRB(ushort extFormat);
ImageFormat GLenumToImageFormat(ushort internalFormat);
ushort ImageTypeToGLTarget(ImageType type);
ushort CubeFaceToGLTarget(CubeFace cf);
ImageType GLTargetToImageType(ushort gl_Target);

}}
