#pragma once

#include "Core/Core.h"

#include "Core/Range/StringView.h"

#include "Container/ForwardDecls.h"

#include "Math/Math.h"
#include "Math/Matrix3.h"
#include "Math/Matrix4.h"
#include "Math/Vector2.h"
#include "Math/Vector3.h"
#include "Math/Vector4.h"
#include "Math/FixedPoint.h"
#include "Math/HalfFloat.h"

#include "Core/TList.h"

INTRA_BEGIN
inline namespace Data {

struct ValueType
{
	enum I: byte
	{
		Void = 0,

		//Vectors \ scalars
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
		SNorm24, S24Vec2, S24Vec3, S24Vec4,
		SNorm32, S32Vec2, S32Vec3, S32Vec4,

		NVec233, NVec565, NVec1555, NVec4444, Vec10n10n10n2n, Vec10s10s10s2s,
		NVec332, NVec5551, N10Vec3, N5Vec3,
		
		//Matrices
		FMat2, FMat2x3, FMat2x4, FMat3x2, FMat3, FMat3x4, FMat4x2, FMat4x3, FMat4,
		Mat2 = FMat2, Mat2x3 = FMat2x3, Mat2x4 = FMat2x4, Mat3x2 = FMat3x2, Mat3 = FMat3, Mat3x3 = Mat3,
		FMat3x3 = Mat3, Mat3x4 = FMat3x4, Mat4x2 = FMat4x2, Mat4x3 = FMat4x3, Mat4 = FMat4, FMat4x4 = Mat4, Mat4x4 = Mat4,

		DMat2, DMat2x3, DMat2x4, DMat3x2, DMat3, DMat3x4, DMat4x2, DMat4x3, DMat4,

		Char,
		String, StringView, StructureInstance, StructureType,

		First = Float, FirstOfVectors=Float, FirstOfFloat = FirstOfVectors, EndOfFloat=HVec4+1,
		FirstOfPackedFloat = Vec11f11f10f, EndOfPackedFloat = Vec11f11f10f+1,
		FirstOfInteger = Int, EndOfInteger = UBVec4+1,
		FirstOfPackedInt = UVec4444, EndOfPackedInt = UVec9995+1,
		FirstOfNormalized = Norm8, EndOfNormalized = S32Vec4+1,
		FirstOfPackedNorm = NVec233, EndOfPackedNorm = N5Vec3+1, EndOfVectors = EndOfPackedNorm,
		FirstOfMatrices = FMat2, EndOfMatrices = Char, EndOfPod = String, End = StructureType+1
	};

	I value;

	explicit constexpr forceinline ValueType(uint vt): value(I(vt)) {}
	constexpr forceinline ValueType(I vt): value(vt) {}
	constexpr forceinline ValueType(null_t=null): value(Void) {}
	constexpr forceinline ValueType(const ValueType& rhs): value(rhs.value) {}

	forceinline ValueType& operator=(const ValueType& rhs) {value = rhs.value; return *this;}
	forceinline ValueType& operator=(ValueType::I rhs) {value = rhs; return *this;}
	forceinline ValueType& operator=(null_t) {value = Void; return *this;}
	INTRA_NODISCARD constexpr forceinline bool operator==(null_t) const {return value == Void;}
	INTRA_NODISCARD constexpr forceinline bool operator!=(null_t) const {return !operator==(null);}
	INTRA_NODISCARD constexpr forceinline bool operator==(byte rhs) const {return value == rhs;}
	INTRA_NODISCARD constexpr forceinline bool operator!=(byte rhs) const {return !operator==(rhs);}
	INTRA_NODISCARD constexpr forceinline bool operator==(const ValueType& rhs) const {return value == rhs.value;}
	INTRA_NODISCARD constexpr forceinline bool operator!=(const ValueType& rhs) const {return !operator==(rhs);}

	ushort Size() const;

	INTRA_NODISCARD constexpr bool IsScalar() const noexcept
	{
		return (value == Float || value == Double || value == Half) ||
			(value >= FirstOfInteger && value < EndOfInteger && (value - FirstOfInteger) % 4 == 0) ||
			(value >= FirstOfNormalized && value < EndOfNormalized && (value - FirstOfNormalized) % 4 == 0);
	}

	INTRA_NODISCARD constexpr forceinline bool IsPod() const noexcept {return value < EndOfPod;}

	INTRA_NODISCARD constexpr forceinline bool IsValid() const noexcept {return value != Void && value < End;}
	INTRA_NODISCARD constexpr forceinline bool IsMatrix() const noexcept {return value >= FirstOfMatrices && value < EndOfMatrices;}
	INTRA_NODISCARD constexpr forceinline bool IsVector() const noexcept {return value >= FirstOfVectors && value < EndOfVectors && !IsScalar();}

	INTRA_NODISCARD constexpr bool IsPacked() const
	{
		return (value == Vec11f11f10f ||
			(value >= FirstOfPackedInt && value < EndOfPackedInt) ||
			(value >= FirstOfPackedNorm && value < EndOfPackedNorm));
	}

	INTRA_NODISCARD constexpr bool IsNormalized() const
	{
		return (value >= FirstOfNormalized && value < EndOfNormalized) ||
			(value >= FirstOfPackedNorm && value < EndOfPackedNorm);
	}

	INTRA_NODISCARD constexpr forceinline bool IsInteger() const
	{
		return (value >= FirstOfInteger && value < EndOfInteger) ||
			(value >= FirstOfPackedInt && value < EndOfPackedInt);
	}

	//Возвращает размерность вектора или 1 если это скаляр. Для матриц не работает
	byte Dimensions() const;

	//Возвращает тип элемента вектора или матрицы. Если тип скалярный, то он же и возвращается
	ValueType ToScalarType() const;

	//Переводит все нормализованные типы в float, упакованные и меньшие типы в основные типы (float, double, int, uint, bool)
	ValueType ToShaderType() const;

	Core::StringView ToStringGLSL() const;
	Core::StringView ToString() const;
	static ValueType FromString(Range::StringView str);
	static ValueType FromStringGLSL(Range::StringView str);

	//TODO: Before type list redesign this header added ~22 ms to compilation (84 ms including headers, one of them - TypeList 5 ms)
	//Measure again
	typedef TList<void,

		//Vectors / scalars
		float, Math::Vec2, Math::Vec3, Math::Vec4,

		double, Math::DVec2, Math::DVec3, Math::DVec4,
		Math::HalfFloat, Math::Vector2<Math::HalfFloat>, Math::Vector3<Math::HalfFloat>, Math::Vector4<Math::HalfFloat>,

		null_t,

		int, Math::IVec2, Math::IVec3, Math::IVec4,
		uint, Math::UVec2, Math::UVec3, Math::UVec4,
		short, Math::SVec2, Math::SVec3, Math::SVec4,
		ushort, Math::USVec2, Math::USVec3, Math::USVec4,
		sbyte, Math::SBVec2, Math::SBVec3, Math::SBVec4,
		byte, Math::UBVec2, Math::UBVec3, Math::UBVec4,
		bool, Math::BVec2, Math::BVec3, Math::BVec4,

		null_t, null_t, null_t,

		Math::Norm8s, Math::N8Vec2, Math::N8Vec3, Math::N8Vec4,
		Math::Norm16s, Math::N16Vec2, Math::N16Vec3, Math::N16Vec4,
		Math::Norm32s, Math::N32Vec2, Math::N32Vec3, Math::N32Vec4,
		Math::SNorm8s, Math::S8Vec2, Math::S8Vec3, Math::S8Vec4,
		Math::SNorm16s, Math::S16Vec2, Math::S16Vec3, Math::S16Vec4,
		null_t, null_t, null_t, null_t,
		//Math::SNorm24s, Math::S24Vec2, Math::S24Vec3, Math::S24Vec4,
		Math::SNorm32s, Math::S32Vec2, Math::S32Vec3, Math::S32Vec4,

		null_t, null_t, null_t, null_t, null_t, null_t,
		null_t, null_t, null_t, null_t,

		//Matrices
		/*Math::Mat2*/null_t, null_t, null_t, null_t, Math::Mat3, null_t, null_t, null_t, Math::Mat4,

		/*Math::DMat2*/null_t, null_t, null_t, null_t, Math::DMat3, null_t, null_t, null_t, Math::DMat4,

		char, Container::String, Range::StringView, null_t, null_t> ValueTypeList;

	template<typename T> INTRA_NODISCARD static forceinline constexpr ValueType Of()
	{
		enum {FoundPos = TypeListFind<T, ValueTypeList>};
		return FoundPos == ValueTypeList::Length? ValueType::End: FoundPos;
	}
};

}
INTRA_END
