#include "Imaging/Complex.h"
#include "Imaging/Image.h"
#include "Containers/Array2D.h"

#if INTRA_DISABLED

using namespace Math;


Image CreateMapImage(cfloat(*mapping)(cfloat z), cfloat(*srcPoint)(vec2 xy),
	vec2 xymin, vec2 xymax, uvec2 steps, usvec2 imageSize)
{
	Array2D<ubvec3> planeW(imageSize);
	memset(planeW.GetDataPtr(), 0, planeW.SizeInBytes());

	vec2 step=(xymax-xymin)/steps;
	vec2 xy;
	for(xy.y=xymin.y; xy.y<xymax.y; xy.y+=step.y)
		for(xy.x=xymin.x; xy.x<xymax.x; xy.x+=step.x)
		{
			cfloat z = srcPoint(xy);
			cfloat w = mapping(z);
			z = (z*100.0f+cfloat((float)planeW.Sizes().x, (float)planeW.Sizes().y)/2.0f);
			w = (w*100.0f+cfloat((float)planeW.Sizes().x, (float)planeW.Sizes().y)/2.0f);
			if(w.real()>0 && w.real()<planeW.Sizes().x && w.imag()>0 && w.imag()<planeW.Sizes().y)
				planeW((int)w.real(), (int)w.imag()).x=255;
			if(z.real()>0 && z.real()<planeW.Sizes().x && z.imag()>0 && z.imag()<planeW.Sizes().y)
				planeW((int)z.real(), (int)z.imag()).y=255;
		}

	Image img;
	img.Info = {usvec3((usvec2)planeW.Sizes(), 1), ImageFormat::RGB8, ImageType_2D, 0};
	img.Data = planeW.MoveToByteBuffer();
	return img;
}

#endif
