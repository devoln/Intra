#pragma once

#include "Containers/Array2D.h"
#include "IO/Stream.h"
#include "IO/File.h"

namespace Intra {

class CharMap: public Array2D<char>
{
	typedef Array2D<char> super;
public:
	CharMap(const Array2D<char>& arr);
	CharMap(ArrayRange<const Array<char>> arr, char filler='\0');
	CharMap(ArrayRange<const String> arr, char filler='\0');
	CharMap(const CharMap& rhs): super(rhs) {}
	CharMap(CharMap&& rhs): super(core::move(rhs)) {}

	struct Block
	{
		uint xleft, ytop, width, height;
	};

	enum BlockType {Point, Horizontal, Vertical, HorizontalOrVertical, Area};
	Array<Block> GetBlocks(char c, BlockType type) const;
	Array<Math::UVec2> GetObjects(char c) const;

	static CharMap FromStream(IO::IInputStream& s);
	static CharMap FromFile(StringView filename);

	CharMap& operator=(const CharMap& rhs) {super::operator=(rhs); return *this;}
	CharMap& operator=(CharMap&& rhs) {super::operator=(core::move((super&)rhs)); return *this;}
};

}
