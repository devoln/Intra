#include "Text/CharMap.h"

namespace Intra {

using namespace Math;

CharMap::CharMap(const Array2D<char>& arr):
	Array2D(arr) {}

CharMap::CharMap(ArrayRange<const Array<char>> arr, char filler)
{
	size_t columns=0;
	for(auto& line: arr) columns = Max(columns, line.Count());
	Array2D<char> result({arr.Length(), columns});
	for(size_t i=0; i<arr.Length(); i++)
		for(size_t j=0; j<columns; j++)
			operator()(j, i) = j<arr[i].Count()? arr[i][j]: filler;
	Array2D<char>::operator=(Meta::Move(result));
}

CharMap::CharMap(ArrayRange<const String> arr, char filler)
{
	size_t columns=0;
	for(auto& line: arr) columns = Max(columns, line.Length());
	Array2D<char> result({columns, arr.Length()});
	for(size_t i=0; i<arr.Length(); i++)
		for(size_t j=0; j<columns; j++)
			result(j, i) = j<arr[i].Length()? arr[i][j]: filler;
	Array2D<char>::operator=(Meta::Move(result));
}

Array<CharMap::Block> CharMap::GetBlocks(char c, BlockType type) const
{
	Array<Block> result;

	if(type==Point)
	{
		for(uint i=0; i<Height(); i++)
			for(uint j=0; j<Width(); j++)
			{
				if(operator()(j, i)!=c) continue;
				result.AddLast({j,i,1,1});
			}
		return result;
	}

	Array2D<char> m = *this;

	for(uint i=0; i<Height(); i++)
		for(uint j=0; j<Width(); j++)
		{
			if(m(j, i)!=c) continue;

			uint j2=j+1;
			if(type==Horizontal || type==HorizontalOrVertical || type==Area)
				while(j2<Width() && m(j2, i)==c) j2++;

			uint i2=i+1;

			if( ((type==Vertical || type==HorizontalOrVertical) && j2-j==1) || type==Area )
			{
				if(j2-j==1) while(i2<Height() && m(j, i2)==c) i2++;
				else while(i2<Height() && C::memcmp(&m(j, i2-1), &m(j, i2), j2-j)==0) i2++;
			}

			for(uint ii=i; ii<i2; ii++)
				for(uint jj=j; jj<j2; jj++)
					m(jj, ii)='\0';

			result.AddLast({j,i, j2-j, i2-i});
		}

	return result;
}

Array<UVec2> CharMap::GetObjects(char c) const
{
	Array<UVec2> result;
	for(uint i=0; i<Height(); i++)
		for(uint j=0; j<Width(); j++)
		{
			if(operator()(j, i)!=c) continue;
			result.AddLast({j,i});
		}
	return result;
}

CharMap CharMap::FromStream(IO::IInputStream& s)
{
	Array<String> lines;
	while(!s.EndOfStream())
		lines.AddLast(s.ReadLine());
	return CharMap(lines);
}

CharMap CharMap::FromFile(StringView filename)
{
	IO::DiskFile::Reader file(filename);
	return FromStream(file);
}

}

