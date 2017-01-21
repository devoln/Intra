#include "Imaging/Loaders/LoaderGIF.h"
#include "Imaging/Loaders/LoaderPlatform.h"
#include "Imaging/Image.h"

namespace Intra { namespace Imaging {

bool LoaderGIF::IsValidHeader(const void* header, size_t headerSize) const
{
	const byte* headerBytes = reinterpret_cast<const byte*>(header);
	return headerSize>=3 &&
		headerBytes[0]=='G' && headerBytes[1]=='I' && headerBytes[2]=='F';
}

Image LoaderGIF::Load(IO::IInputStream& stream, size_t bytes) const
{
#if(INTRA_LIBRARY_IMAGE_LOADING!=INTRA_LIBRARY_IMAGE_LOADING_None)
	return LoadWithPlatform(stream, bytes);
#else
	(void)stream;
	(void)bytes;
	return null;
#endif
}

}}
