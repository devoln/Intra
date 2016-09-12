#pragma once

#include "IO/Stream.h"
#include "Imaging/ImagingTypes.h"
#include "Math/Vector.h"
#include "Containers/Array.h"

namespace Intra {

class Image
{
public:
	Image(null_t=null) {}

	Image(const Image& rhs) = default;
	Image(Image&& rhs): Data(core::move(rhs.Data)), Info(rhs.Info),
		SwapRB(rhs.SwapRB), LineAlignment(rhs.LineAlignment) {}
	Image(Math::USVec3 size, ImageFormat format, ushort mipmapCount=0, ImageType type=ImageType_2D):
		Info(size, format, type, mipmapCount), SwapRB(false), LineAlignment(1) {}

	Image& operator=(const Image& rhs) = default;

	Image& operator=(Image&& rhs)
	{
		Data = core::move(rhs.Data);
		Info = rhs.Info;
		SwapRB=  rhs.SwapRB;
		LineAlignment = rhs.LineAlignment;
		return *this;
	}

	bool operator==(null_t) const {return Data==null;}
	bool operator!=(null_t) const {return !operator==(null);}

	Image ExtractChannel(char channelName, ImageFormat compatibleFormat, ushort newLineAlignment=0) const;

	static Image FromData(Math::USVec3 size, ImageFormat format, ImageType type, const void* data,
		ushort borderLeft, ushort borderTop, ushort borderRight, ushort borderBottom);

	Array<byte> Data;
	ImageInfo Info;
	bool SwapRB=false;
	byte LineAlignment=1;

	const void* GetMipmapDataPtr(ushort mip) const;
	void* GetMipmapDataPtr(ushort mip)
	{
		return const_cast<void*>( const_cast<const Image*>(this)->GetMipmapDataPtr(mip) );
	}

	Array<const void*> GetMipmapPointers() const;

#ifndef NO_IMAGE_LOADING

	enum class FileFormat: ushort {JPEG, BMP, GIF, TIFF, DDS, PNG, TGA, KTX, Unknown};
	static FileFormat DetectFileFormatByHeader(byte header[12]);
	static ImageInfo GetImageInfo(IO::IInputStream* s, FileFormat* format=null);
	static ImageInfo GetImageInfo(StringView filename, FileFormat* format=null);

	static Image FromFile(StringView filename);
	void SaveToFileDDS(StringView filename) const;

	static Image FromStream(IO::IInputStream* s, uint bytes)
	{
		Image result;
		result.load(s, bytes);
		return result;
	}

	void ToStream(IO::IOutputStream* s) const
	{
		saveDDS(s);
	}

private:
	void load(IO::IInputStream* s, uint bytes);

#ifndef NO_KTX_LOADER
	void loadKTX(IO::IInputStream* s, uint bytes);
#endif
#ifndef INTRA_NO_DDS_LOADER
	void loadDDS(IO::IInputStream* s, uint bytes);
	void saveDDS(IO::IOutputStream* s) const;
#endif
#ifndef INTRA_NO_BMP_LOADER
	void loadBMP(IO::IInputStream* s, uint bytes);
#endif
#ifndef INTRA_NO_TGA_LOADER
	void loadTGA(IO::IInputStream* s, uint bytes);
#endif
	void load_with_library(IO::IInputStream* s, uint bytes);

#endif
};

}
