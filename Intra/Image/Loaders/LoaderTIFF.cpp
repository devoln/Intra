#include "Image/Loaders/LoaderTIFF.h"
#include "Image/Loaders/LoaderPlatform.h"
#include "Image/AnyImage.h"

namespace Intra { namespace Image {

bool LoaderTIFF::IsValidHeader(const void* header, size_t headerSize) const
{
	const byte* headerBytes = reinterpret_cast<const byte*>(header);
	return headerSize>=3 && headerBytes[0]=='I' && headerBytes[1]=='I' && headerBytes[2]=='*';
}



AnyImage LoaderTIFF::Load(IInputStream& stream) const
{
#if(INTRA_LIBRARY_IMAGE_LOADING != INTRA_LIBRARY_IMAGE_LOADING_None)
	return LoadWithPlatform(stream);
#else
	(void)stream;
	return null;
#endif
}

}}
