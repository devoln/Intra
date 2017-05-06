#include "Image/Loaders/LoaderGIF.h"
#include "Image/Loaders/LoaderPlatform.h"
#include "Image/AnyImage.h"

namespace Intra { namespace Image {

bool LoaderGIF::IsValidHeader(const void* header, size_t headerSize) const
{
	const byte* headerBytes = reinterpret_cast<const byte*>(header);
	return headerSize>=3 &&
		headerBytes[0]=='G' && headerBytes[1]=='I' && headerBytes[2]=='F';
}

AnyImage LoaderGIF::Load(InputStream stream) const
{
#if(INTRA_LIBRARY_IMAGE_LOADING!=INTRA_LIBRARY_IMAGE_LOADING_None)
	return LoadWithPlatform(Cpp::Move(stream));
#else
	(void)stream;
	return null;
#endif
}

}}
