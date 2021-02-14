#include "IntraX/Unstable/Image/Loaders/LoaderTIFF.h"
#include "IntraX/Unstable/Image/Loaders/LoaderPlatform.h"
#include "IntraX/Unstable/Image/AnyImage.h"

namespace Intra { INTRA_BEGIN
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
	return nullptr;
#endif
}
} INTRA_END
