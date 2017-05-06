#pragma once

#include "Utils/Span.h"
#include "Range/Operations.h"
#include "Range/Generators/FlatArrayOfArraysRange.h"
#include "Range/Stream/RawRead.h"

namespace Intra { namespace Data { namespace Format {

class RawType;

class RawTypeRange
{
public:
	forceinline RawTypeRange(const void* data, size_t size): mData(data, size) {}
	forceinline RawTypeRange(CSpan<byte> data): mData(data) {}

	forceinline const RawType& First() const {return *reinterpret_cast<const RawType*>(mData.First().Data());}
	forceinline size_t FirstLength() const {return mData.First().Length();}
	forceinline void PopFirst() {mData.PopFirst();}
	forceinline bool Empty() const {return mData.Empty();}

	const byte* RawData() const {return mData.RawData();}
	const byte* RawEnd() const {return mData.RawEnd();}
	CSpan<byte> RawRange() const {return mData.RawRange();}

private:
	Range::FlatArrayOfArraysRange<const byte> mData;
};

class RawStruct
{
public:
	Range::FlatConstStringRange FieldNames(const byte* blockEnd) const
	{
		CSpan<byte> s = {mData, blockEnd};
		const size_t sizeInBytes = size_t(ParseVarUInt(s));
		return Range::FlatConstStringRange(s.Take(sizeInBytes));
	}

	RawTypeRange FieldTypes(const byte* blockEnd) const
	{
		CSpan<byte> s = {FieldNames(blockEnd).RawEnd(), blockEnd};
		const size_t sizeInBytes = size_t(ParseVarUInt(s));
		return RawTypeRange(s.Take(sizeInBytes));
	}

	forceinline size_t FieldCount(const byte* blockEnd) const {return Count(FieldNames(blockEnd));}

	forceinline const byte* DefaultValuePtr(const byte* blockEnd) const {return FieldTypes(blockEnd).RawEnd();}

	template<typename T> forceinline Meta::EnableIf<
		Meta::IsTriviallySerializable<T>::_,
	const T&> DefaultValueAsUnchecked() const
	{return *reinterpret_cast<const T*>(DefaultValuePtr());}

private:
	RawStruct() = delete;
	RawStruct(const RawStruct&) = delete;
	~RawStruct() = delete;
	RawStruct& operator=(const RawStruct&) = delete;

	//Out of bounds indexing in this class is a feature, not a bug.
	byte mData[1];
};

class RawReference
{

private:
	RawReference() = delete;
	RawReference(const RawReference&) = delete;
	~RawReference() = delete;
	RawReference& operator=(const RawReference&) = delete;

	//Out of bounds indexing in this class is a feature, not a bug.
	byte mData[1];
};

//! “ип данных, записанный в бинарном виде.
/*! ќбъект этого класса не €вл€етс€ самодостаточным и может содержать относительные ссылки на другие типы.
 ≈го методы можно использовать только в случае если он имеет базовый тип, или
если адрес this находитс€ в контексте, в котором эти относительные ссылки верны.
 Ќарушение этого правила приводит к undefined behaviour!
  ак правило этот объект получаетс€ через reinterpret_cast указател€ на данные бинарной структуры.
*/
class RawType
{
public:
	//! ѕервый байт любого типа, который описывает его категорию и содержит информацию о том, как его парсить дальше.
	enum: byte {
	//! Basic types.
	Void=0x00, SByte=0x01, Byte, Short, UShort, Int, UInt, Long, ULong, Float, Double, Bool,
	BasicTypeMask = 0x0F,
	
	//! Basic fixed point types.
	//! Then: varuint divisor.
	FPByte=0x11, FPSByte, FPShort, FPUShort, FPInt, FPUInt, FPLong, FPULong,
	
	//! Fixed-length array.
	//! Then: type elementType;
	//! Then: varuint sizeInBytes;
	//! Value: elementType[elementCount] elements.
	FixedArray=0x20,

	//! Fixed-length array of basic type.
	//! Then: varuint elementCount;
	//! Value: elementType[elementCount] elements.
	FixedArraySByte=FixedArray|SByte, FixedArrayByte, FixedArrayShort, FixedArrayUShort,
	FixedArrayInt, FixedArrayUInt, FixedArrayLong, FixedArrayULong,
	FixedArrayFloat, FixedArrayDouble, FixedArrayBool,

	//! Variable length array.
	//! Then: type elementType.
	//! Value:
	//!   varuint sizeInBytes;
	//!   elementType[<element count>] elements.
	VarArray=0x30,

	//! Variable length array of basic type.
	//! Value:
	//!   varuint elementCount;
	//!   elementType[elementCount] elements.
	VarArraySByte=VarArray|SByte, VarArrayByte, VarArrayShort, VarArrayUShort,
	VarArrayInt, VarArrayUInt, VarArrayLong, VarArrayULong,
	VarArrayFloat, VarArrayDouble, VarArrayBool,

	//! Map (associative array)
	//! Then: type keyType.
	//! Then: type valueType.
	//! Value:
	//!   varuint elementCount;
	//!   keyType[elementCount] keys;
	//!   valueType[elementCount] values - sorted by keys.
	Dictionary=0xF2,

	//! Reference to value.
	//! Then: type referencedValueType.
	//! Value:
	//!   byte importIndex - index of external imported struct;
	//!   importIndex==0: varint relativeOffsetOfValueInBytes;
	//!   importIndex!=0: varuint absoluteOffsetOfValueInBytes.
	Reference=0xF3,

	//! Definition of structure.
	//! Value:
	//!   byte[][] fieldNames;
	//!   type[] fieldTypes;
	//!   <default field values as struct instance>.
	Struct=0xF4,

	//! Instance of struct.
	//! Then: byte importIndex;
	//! Then: importIndex==0: varint relativeOffsetOfValueInBytes;
	//!       importIndex!=0: varuint absoluteOffsetOfValueInBytes.
	//! Value:
	//!   <values of fields in order of their declarations>
	StructInstance=0xF5,

	//! Type.
	//! Value:
	//!   varuint sizeInBytes;
	//!   Any type definition as DataType + all "Then's".
	Type=0xF6
	};

	forceinline bool IsStruct() const {return mData[0]==Struct;}

	forceinline bool IsFixedPoint() const {return mData[0] >= FPByte && mData[0] <= FPULong;}

	ulong64 FixedPointDivisor(const byte* blockEnd) const
	{
		INTRA_DEBUG_ASSERT(IsFixedPoint());
		CSpan<byte> s = {mData+1, blockEnd};
		return ParseVarUInt(s);
	}
	
	const RawStruct& AsStruct() const
	{
		INTRA_DEBUG_ASSERT(IsStruct());
		return *reinterpret_cast<const RawStruct*>(mData+1);
	}

	forceinline bool IsGenericFixedArray() const {return mData[0] == FixedArray;}

	forceinline bool IsFixedArrayOf(byte type) const
	{
		if((type & BasicTypeMask) != type) return false;
		return (mData[0] == (FixedArray|type)) ||
			(mData[0] == FixedArray && mData[1] == 1 && mData[2] == type);
	}

	forceinline bool IsReference() const {return mData[0] == Reference;}

	const RawType& ReferencedType() const
	{
		INTRA_DEBUG_ASSERT(IsReference());
		return *reinterpret_cast<const RawType*>(mData+1);
	}

private:
	RawType() = delete;
	~RawType() = delete;
	RawType(const RawType&) = delete;
	RawType& operator=(const RawType&) = delete;

	//Out of bounds indexing in this class is a feature, not a bug.
	byte mData[1];
};

#define DEFINE(type, enum) \
	forceinline bool TypesEqual(CSpan<byte> s, type) {return s.First() == RawType::enum;} \
	forceinline byte RawTypeOf(type) {return RawType::enum;}
DEFINE(sbyte, SByte);
DEFINE(short, Short);
DEFINE(ushort, UShort);
DEFINE(int, Int);
DEFINE(uint, UInt);
DEFINE(long64, Long);
DEFINE(ulong64, ULong);
DEFINE(float, Float);
DEFINE(double, Double);
DEFINE(bool, Bool);
#undef DEFINE

template<typename T, size_t N> forceinline bool TypesEqual(CSpan<byte> s, T(&arr)[N])
{
	//if(s.First() == RawType::VarArray || s.First() == (RawType::VarArray|RawTypeOf(arr[0]))) return true;
	if(s.First() == RawType::FixedArray)
	{
		s.PopFirst();
		return ParseVarUInt(s) == N;
	}
	return false;
}


constexpr forceinline size_t VarUIntBytes(ulong64 x) {return x>=0x80? 1+VarUIntBytes(x >> 7): 1;}

template<typename T, size_t N> CSpan<byte> RawTypeOf(T(&arr)[N])
{
	static const byte result[] = {RawTypeOf(arr[0])|RawType::FixedArray, };
	return result;
}




const RawType& ParseType(CSpan<byte>& range)
{
	byte signature = range.First();
	range.PopFirst();

	if(signature <= RawType::Bool) return;
	
	if(signature >= RawType::FPByte && signature <= RawType::FPULong)
	{
		const ulong64 divisor = ParseVarUInt(range);
		return;
	}

	if(signature == RawType::FixedArray)
	{
		const ulong64 sizeInBytes = ParseVarUInt(range);
		ParseType(range);
		return;
	}

	if(signature & RawType::FixedArray == RawType::FixedArray)
	{
		const ulong64 elementCount = ParseVarUInt(range);
		return;
	}

	if(signature == RawType::VarArray)
	{
		ParseType(range);
		return;
	}

	if((signature & RawType::VarArray) == RawType::VarArray)
	{
		return;
	}

	if(signature == RawType::Dictionary)
	{
		ParseType(range);
		ParseType(range);
		return;
	}

	if(signature == RawType::Reference)
	{
		ParseType(range);
		return;
	}
}


}}}
