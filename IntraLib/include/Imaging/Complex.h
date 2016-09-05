#pragma once

#if DISABLED

#include "Image.h"
#include <complex>
typedef std::complex<float> cfloat;

//Image CreateMapImage(cfloat(*mapping)(cfloat), cfloat(*srcPoint)(Math::vec2),
//	Math::vec2 xymin, Math::vec2 xymax, uvec2 steps, Math::usvec2 imageSize);

#endif
