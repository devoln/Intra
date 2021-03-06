﻿#pragma once

#include "Cpp/Warnings.h"
#include "Cpp/Fundamental.h"

#include "Math/Vector2.h"
#include "Math/Vector3.h"
#include "Math/Vector4.h"
#include "Math/Matrix3.h"
#include "Math/Matrix4.h"
#include "Math/HalfFloat.h"

#include "Data/ValueType.h"
#include "Utils/FixedArray.h"
#include "Container/Sequential/Array.h"

INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS

namespace Intra { namespace Data {

struct Variable
{
	union
	{
		Math::HalfFloat AsHalf;
		Math::Vector2<Math::HalfFloat> AsHVec2;
		Math::Vector3<Math::HalfFloat> AsHVec3;
		Math::Vector4<Math::HalfFloat> AsHVec4;

		float AsFloat;
		Math::Vec2 AsVec2;
		Math::Vec3 AsVec3;
		Math::Vec4 AsVec4;

		double AsDouble;
		Math::DVec2 AsDVec2;
		Math::DVec3 AsDVec3;
		Math::DVec4 AsDVec4;

		int AsInt;
		Math::IVec2 AsIVec2;
		Math::IVec3 AsIVec3;
		Math::IVec4 AsIVec4;

		uint AsUInt;
		Math::IVec2 AsUVec2;
		Math::IVec3 AsUVec3;
		Math::IVec4 AsUVec4;

		short AsShort;
		Math::SVec2 AsSVec2;
		Math::SVec3 AsSVec3;
		Math::SVec4 AsSVec4;

		ushort AsUShort;
		Math::USVec2 AsUSVec2;
		Math::USVec3 AsUSVec3;
		Math::USVec4 AsUSVec4;

		sbyte AsSByte;
		Math::SBVec2 AsSBVec2;
		Math::SBVec3 AsSBVec3;
		Math::SBVec4 AsSBVec4;

		byte AsByte;
		Math::UBVec2 AsUBVec2;
		Math::UBVec3 AsUBVec3;
		Math::UBVec4 AsUBVec4;

		Math::Mat3 AsMat3;
		Math::Mat4 AsMat4;

		char AsString[sizeof(Math::Mat4)];
		bool AsBool;
		Math::BVec2 AsBVec2;
		Math::BVec3 AsBVec3;
		Math::BVec4 AsBVec4;
	};

	static Variable Null()
	{
		Variable result;
		C::memset(&result, 0, sizeof(result));
		return result;
	}

	void ConvertType(Variable& dst, ValueType srcType, ValueType dstType) const;
	void ConvertTypeFromDVec4(Variable& dst, ValueType dstType) const;
};

class VariableArray
{
public:
	VariableArray(): mData(null), mVariables(null) {}

	const Variable& operator[](size_t index) const
	{return *reinterpret_cast<const Variable*>(mData.Data() + mVariables[index].Offset);}

	template<typename T> T& Get(size_t index, size_t arrIndex)
	{return *reinterpret_cast<T*>(mData.Data() + mVariables[index].Offset + arrIndex*sizeof(T));}

	template<typename T> const T& Get(size_t index, size_t arrIndex) const
	{return reinterpret_cast<const T*>(GetPtr(index))[arrIndex];}

	const void* GetPtr(size_t index) const
	{return mData.Data() + mVariables[index].Offset;}

	AnyPtr GetPtr(size_t index)
	{return mData.Data() + mVariables[index].Offset;}

	void Add(const void* value, size_t bytes)
	{
		mVariables.EmplaceLast(uint(mData.Count()));
		mData.AddLastRange(CSpan<byte>(static_cast<const byte*>(value), bytes));
	}

	void* AddData(size_t bytes)
	{
		mVariables.EmplaceLast(uint(mData.Count()));
		mData.SetCountUninitialized(mData.Count()+bytes);
		return mData.end()-bytes;
	}

	void Set(size_t index, const void* arrData, size_t start, size_t bytes)
	{C::memcpy(&mData[mVariables[index].Offset+start], arrData, bytes);}

	template<typename T> void Add(const T& value)
	{
		mVariables.EmplaceLast(uint(mData.Count()));
		mData.AddLastRange(CSpan<byte>(reinterpret_cast<const byte*>(&value), sizeof(T)));
	}

	void Add(const Variable& value, ValueType type)
	{
		mVariables.EmplaceLast(uint(mData.Count()));
		mData.AddLastRange(CSpan<byte>(reinterpret_cast<const byte*>(&value), type.Size()));
	}

	template<typename T> void Set(size_t index, CSpan<T> arr, size_t first=0)
	{Set(index, arr.Data(), first*sizeof(T), arr.Length()*sizeof(T));}

	void ReserveBytes(size_t bytes) {mData.Reserve(bytes);}

	bool operator==(const VariableArray& rhs) const
	{return mData == rhs.mData && mVariables == rhs.mVariables;}

	bool operator==(null_t) const {return mData==null;}
	bool operator!=(null_t) const {return !operator==(null);}

	size_t VariableCount() const {return mVariables.Count();}
	size_t DataSizeInBytes() const {return mData.Capacity();}
	Array<byte>& Data() {return mData;}
	const Array<byte>& Data() const {return mData;}

private:
	struct VarEntry
	{
		VarEntry() = default;
		VarEntry(uint offset): Offset(offset) {}

		bool operator==(const VarEntry& rhs) const {return Offset==rhs.Offset;}
		bool operator!=(const VarEntry& rhs) const {return !operator==(rhs);}

		uint Offset;
	};

	Array<byte> mData;
	Array<VarEntry> mVariables;
};

}}

INTRA_WARNING_POP
