#pragma once

#include "Containers/Array2D.h"
#include "IO/Stream.h"
#include "IO/File.h"

namespace Intra {

class CharMap: public Array2D<char>
{
public:
	CharMap(const Array2D<char>& arr);
	CharMap(ArrayRange<const Array<char>> arr, char filler='\0');
	CharMap(ArrayRange<const String> arr, char filler='\0');
	CharMap(const CharMap& rhs): Array2D<char>(rhs) {}
	CharMap(CharMap&& rhs): Array2D<char>(Meta::Move(rhs)) {}

	struct Block
	{
		uint xleft, ytop, width, height;
	};

	enum BlockType {Point, Horizontal, Vertical, HorizontalOrVertical, Area};
	Array<Block> GetBlocks(char c, BlockType type) const;
	Array<Math::UVec2> GetObjects(char c) const;

	static CharMap FromStream(IO::IInputStream& s);
	static CharMap FromFile(StringView filename);

	CharMap& operator=(const CharMap& rhs)
	{
		Array2D<char>::operator=(rhs);
		return *this;
	}

	CharMap& operator=(CharMap&& rhs)
	{
		Array2D<char>::operator=(static_cast<Array2D<char>&&>(rhs));
		return *this;
	}
};

}
