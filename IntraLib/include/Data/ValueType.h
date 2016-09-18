#pragma once

#include "Containers/ForwardDeclarations.h"
#include "Math/MathEx.h"
#include "Math/Matrix.h"
#include "Meta/TypeList.h"
#include "Math/Fixed.h"
#include "Memory/Allocator.h"

namespace Intra {

struct ValueType
{
	enum I: byte
	{
		Void=0,

		//Векторы\скаляры
		Float,  FVec2, FVec3, FVec4,
		Vec2=FVec2, Vec3=FVec3, Vec4=FVec4,

		Double, DVec2, DVec3, DVec4,
		Half,   HVec2, HVec3, HVec4,

		Vec11f11f10f,

		Int,    IVec2,  IVec3,  IVec4,
		UInt,   UVec2,  UVec3,  UVec4,
		Short,  SVec2,  SVec3,  SVec4,
		UShort, USVec2, USVec3, USVec4,
		SByte,  SBVec2, SBVec3, SBVec4,
		Byte,   UBVec2, UBVec3, UBVec4,
		Bool, BVec2, BVec3, BVec4,

		UVec4444, Vec10u10u10u2u, UVec9995,
				
		Norm8,   N8Vec2,  N8Vec3,  N8Vec4,
		Norm16,  N16Vec2, N16Vec3, N16Vec4,
		Norm32,  N32Vec2, N32Vec3, N32Vec4,
		SNorm8,  S8Vec2,  S8Vec3,  S8Vec4,
		SNorm16, S16Vec2, S16Vec3, S16Vec4,
		SNorm32, S32Vec2, S32Vec3, S32Vec4,

		NVec233, NVec565, NVec1555, NVec4444, Vec10n10n10n2n, Vec10s10s10s2s,
		NVec332, NVec5551, N10Vec3, N5Vec3,
		
		//Матрицы
		FMat2, FMat2x3, FMat2x4, FMat3x2, FMat3, FMat3x4, FMat4x2, FMat4x3, FMat4,
		Mat2=FMat2, Mat2x3=FMat2x3, Mat2x4=FMat2x4, Mat3x2=FMat3x2, Mat3=FMat3, Mat3x3=Mat3,
		FMat3x3=Mat3, Mat3x4=FMat3x4, Mat4x2=FMat4x2, Mat4x3=FMat4x3, Mat4=FMat4, FMat4x4=Mat4, Mat4x4=Mat4,

		DMat2, DMat2x3, DMat2x4, DMat3x2, DMat3, DMat3x4, DMat4x2, DMat4x3, DMat4,

		Char,
		String, StringView, StructureInstance, StructureType,

		First = Float, FirstOfVectors=Float, FirstOfFloat=FirstOfVectors, EndOfFloat=HVec4+1,
		FirstOfPackedFloat=Vec11f11f10f, EndOfPackedFloat=Vec11f11f10f+1,
		FirstOfInteger=Int, EndOfInteger=UBVec4+1,
		FirstOfPackedInt=UVec4444, EndOfPackedInt=UVec9995+1,
		FirstOfNormalized=Norm8, EndOfNormalized=S32Vec4+1,
		FirstOfPackedNorm=NVec233, EndOfPackedNorm=N5Vec3+1, EndOfVectors=EndOfPackedNorm,
		FirstOfMatrices=FMat2, EndOfMatrices=Char, EndOfPod=String, End=StructureType+1
	};

	I value;

	explicit constexpr ValueType(uint vt): value(I(vt)) {}
	constexpr ValueType(I vt): value(vt) {}
	constexpr ValueType(null_t=null): value(Void) {}
	constexpr ValueType(const ValueType& rhs): value(rhs.value) {}

	ValueType& operator=(const ValueType& rhs) {value=rhs.value; return *this;}
	ValueType& operator=(ValueType::I rhs) {value=rhs; return *this;}
	ValueType& operator=(null_t) {value=Void; return *this;}
	constexpr bool operator==(null_t) const {return value==Void;}
	constexpr bool operator!=(null_t) const {return !operator==(null);}
	constexpr bool operator==(byte rhs) const {return value==rhs;}
	constexpr bool operator!=(byte rhs) const {return !operator==(rhs);}
	constexpr bool operator==(const ValueType& rhs) const {return value==rhs.value;}
	constexpr bool operator!=(const ValueType& rhs) const {return !operator==(rhs);}

	ushort Size() const;

	constexpr bool IsScalar() const
	{
		return (value==Float || value==Double || value==Half) ||
			(value>=FirstOfInteger && value<EndOfInteger && (value-FirstOfInteger)%4==0) ||
			(value>=FirstOfNormalized && value<EndOfNormalized && (value-FirstOfNormalized)%4==0);
	}

	constexpr bool IsPod() const {return value<EndOfPod;}

	constexpr bool IsValid() const {return value!=Void && value<End;}
	constexpr bool IsMatrix() const {return value>=FirstOfMatrices && value<EndOfMatrices;}
	constexpr bool IsVector() const {return value>=FirstOfVectors && value<EndOfVectors && !IsScalar();}

	constexpr bool IsPacked() const
	{
		return (value==Vec11f11f10f ||
			(value>=FirstOfPackedInt && value<EndOfPackedInt) ||
			(value>=FirstOfPackedNorm && value<EndOfPackedNorm));
	}

	constexpr bool IsNormalized() const
	{
		return (value>=FirstOfNormalized && value<EndOfNormalized) ||
			(value>=FirstOfPackedNorm && value<EndOfPackedNorm);
	}

	constexpr bool IsInteger() const
	{
		return (value>=FirstOfInteger && value<EndOfInteger) ||
			(value>=FirstOfPackedInt && value<EndOfPackedInt);
	}

	//Возвращает размерность вектора или 1 если это скаляр. Для матриц не работает
	byte Dimensions() const;

	//Возвращает тип элемента вектора или матрицы. Если тип скалярный, то он же и возвращается
	ValueType ToScalarType() const;

	//Переводит все нормализованные типы в float, упакованные и меньшие типы в основные типы (float, double, int, uint, bool)
	ValueType ToShaderType() const;

	Intra::StringView ToStringGLSL() const;
	Intra::StringView ToString() const;
	static ValueType FromString(Intra::StringView str);
	static ValueType FromStringGLSL(Intra::StringView str);

	typedef Meta::TypeList<void,

		//Векторы\скаляры
		float, Math::Vec2, Math::Vec3, Math::Vec4,

		double, Math::DVec2, Math::DVec3, Math::DVec4,
		Math::Half, Math::HVec2, Math::HVec3, Math::HVec4,

		null_t,

		int, Math::IVec2, Math::IVec3, Math::IVec4,
		uint, Math::UVec2, Math::UVec3, Math::UVec4,
		short, Math::SVec2, Math::SVec3, Math::SVec4,
		ushort, Math::USVec2, Math::USVec3, Math::USVec4,
		sbyte, Math::SBVec2, Math::SBVec3, Math::SBVec4,
		byte, Math::UBVec2, Math::UBVec3, Math::UBVec4,
		bool, Math::BVec2, Math::BVec3, Math::BVec4,

		null_t, null_t, null_t,

		norm8s, Math::N8Vec2, Math::N8Vec3, Math::N8Vec4,
		norm16s, Math::N16Vec2, Math::N16Vec3, Math::N16Vec4,
		norm32s, Math::N32Vec2, Math::N32Vec3, Math::N32Vec4,
		snorm8s, Math::S8Vec2, Math::S8Vec3, Math::S8Vec4,
		snorm16s, Math::S16Vec2, Math::S16Vec3, Math::S16Vec4,
		snorm32s, Math::S32Vec2, Math::S32Vec3, Math::S32Vec4,

		null_t, null_t, null_t, null_t, null_t, null_t,
		null_t, null_t, null_t, null_t,

		//Матрицы
		/*Math::Mat2*/null_t, null_t, null_t, null_t, Math::Mat3, null_t, null_t, null_t, Math::Mat4,

		/*Math::DMat2*/null_t, null_t, null_t, null_t, Math::DMat3, null_t, null_t, null_t, Math::DMat4,

		char, Intra::String, Intra::StringView, null_t, null_t> ValueTypeList;

	template<typename T> static constexpr ValueType Of()
	{
		return Meta::TypeListContains<T, ValueTypeList>::_?
			ValueType(uint(Meta::TypeListFind<T, ValueTypeList>::_)):
			ValueType(ValueType::End);
	}
};

}
