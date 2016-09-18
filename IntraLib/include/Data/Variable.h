#pragma once

#include "Core/Core.h"
#include "Math/Vector.h"
#include "Math/Matrix.h"
#include "Data/ValueType.h"
#include "Containers/Array.h"

namespace Intra {

struct Variable
{
	union
	{
		Math::Half AsHalf;
		Math::HVec2 AsHVec2;
		Math::HVec3 AsHVec3;
		Math::HVec4 AsHVec4;

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
		core::memset(&result, 0, sizeof(result));
		return result;
	}

	void ConvertType(Variable& dst, ValueType srcType, ValueType dstType) const;
	void ConvertTypeFromDVec4(Variable& dst, ValueType dstType) const;
};

class VariableArray
{
public:
	VariableArray(): data(null), variables(null) {}
	VariableArray(const VariableArray& rhs) = default;
	VariableArray(VariableArray&& rhs): data(core::move(rhs.data)), variables(core::move(rhs.variables)) {}

	VariableArray& operator=(const VariableArray& rhs) = default;
	VariableArray& operator=(VariableArray&& rhs)
	{
		data = core::move(rhs.data);
		variables = core::move(rhs.variables);
		return *this;
	}

	const Variable& operator[](size_t index) const
	{
		return *reinterpret_cast<const Variable*>(data.Data() + variables[index].Offset);
	}

	template<typename T> T& Get(size_t index, size_t arrIndex)
	{
		return *reinterpret_cast<T*>(data.Data()+variables[index].Offset+arrIndex*sizeof(T));
	}

	template<typename T> const T& Get(size_t index, size_t arrIndex) const
	{
		return reinterpret_cast<const T*>(GetPtr(index))[arrIndex];
	}

	const void* GetPtr(size_t index) const
	{
		return data.Data()+variables[index].Offset;
	}

	AnyPtr GetPtr(size_t index)
	{
		return data.Data()+variables[index].Offset;
	}

	void Add(const void* value, size_t bytes)
	{
		variables.EmplaceLast(uint(data.Count()));
		data.AddLastRange(ArrayRange<const byte>(reinterpret_cast<const byte*>(value), bytes));
	}

	void* AddData(size_t bytes)
	{
		variables.EmplaceLast(uint(data.Count()));
		data.SetCountUninitialized(data.Count()+bytes);
		return data.end()-bytes;
	}

	void Set(size_t index, const void* arrData, size_t start, size_t bytes)
	{
		core::memcpy(&data[variables[index].Offset+start], arrData, bytes);
	}

	template<typename T> void Add(const T& value)
	{
		variables.EmplaceLast(uint(data.Count()));
		data.AddLastRange(ArrayRange<const byte>(reinterpret_cast<const byte*>(&value), sizeof(T)));
	}

	void Add(const Variable& value, ValueType type)
	{
		variables.EmplaceLast(uint(data.Count()));
		data.AddLastRange(ArrayRange<const byte>(reinterpret_cast<const byte*>(&value), type.Size()));
	}

	template<typename T> void Set(size_t index, ArrayRange<const T> arr, size_t first=0)
	{
		Set(index, arr.Data(), first*sizeof(T), arr.Length()*sizeof(T));
	}

	void ReserveBytes(size_t bytes) {data.Reserve(bytes);}

	bool operator==(const VariableArray& rhs) const
	{
		return data==rhs.data && variables==rhs.variables;
	}

	bool operator==(null_t) const {return data==null;}
	bool operator!=(null_t) const {return !operator==(null);}

	size_t VariableCount() const {return variables.Count();}
	size_t DataSizeInBytes() const {return data.Capacity();}
	Array<byte>& Data() {return data;}
	const Array<byte>& Data() const {return data;}

private:
	struct VarEntry
	{
		VarEntry() = default;
		VarEntry(uint offset): Offset(offset) {}
		VarEntry(const VarEntry&) = default;
		VarEntry& operator=(const VarEntry&) = default;

		bool operator==(const VarEntry& rhs) const {return Offset==rhs.Offset;}
		bool operator!=(const VarEntry& rhs) const {return !operator==(rhs);}

		uint Offset;
	};

	Array<byte> data;
	Array<VarEntry> variables;
};

}
