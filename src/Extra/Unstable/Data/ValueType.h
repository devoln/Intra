#pragma once

#include "Intra/Core.h"

#include "Intra/Range/StringView.h"

#include "Extra/Container/ForwardDecls.h"

#include "Intra/Math/Math.h"
#include "Extra/Math/Matrix3.h"
#include "Extra/Math/Matrix4.h"
#include "Extra/Math/Vector2.h"
#include "Extra/Math/Vector3.h"
#include "Extra/Math/Vector4.h"
#include "Intra/Math/FixedPoint.h"
#include "Intra/Math/HalfFloat.h"

#include "Intra/TList.h"

INTRA_BEGIN
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

	explicit constexpr ValueType(unsigned vt): value(I(vt)) {}
	constexpr ValueType(I vt): value(vt) {}
	constexpr ValueType(decltype(null)=null): value(Void) {}
	constexpr ValueType(const ValueType& rhs): value(rhs.value) {}

	ValueType& operator=(const ValueType& rhs) {value = rhs.value; return *this;}
	ValueType& operator=(ValueType::I rhs) {value = rhs; return *this;}
	ValueType& operator=(decltype(null)) {value = Void; return *this;}
	[[nodiscard]] constexpr bool operator==(decltype(null)) const {return value == Void;}
	[[nodiscard]] constexpr bool operator!=(decltype(null)) const {return !operator==(null);}
	[[nodiscard]] constexpr bool operator==(byte rhs) const {return value == rhs;}
	[[nodiscard]] constexpr bool operator!=(byte rhs) const {return !operator==(rhs);}
	[[nodiscard]] constexpr bool operator==(const ValueType& rhs) const {return value == rhs.value;}
	[[nodiscard]] constexpr bool operator!=(const ValueType& rhs) const {return !operator==(rhs);}

	uint16 Size() const;

	[[nodiscard]] constexpr bool IsScalar() const noexcept
	{
		return (value == Float || value == Double || value == Half) ||
			(value >= FirstOfInteger && value < EndOfInteger && (value - FirstOfInteger) % 4 == 0) ||
			(value >= FirstOfNormalized && value < EndOfNormalized && (value - FirstOfNormalized) % 4 == 0);
	}

	[[nodiscard]] constexpr bool IsPod() const noexcept {return value < EndOfPod;}

	[[nodiscard]] constexpr bool IsValid() const noexcept {return value != Void && value < End;}
	[[nodiscard]] constexpr bool IsMatrix() const noexcept {return value >= FirstOfMatrices && value < EndOfMatrices;}
	[[nodiscard]] constexpr bool IsVector() const noexcept {return value >= FirstOfVectors && value < EndOfVectors && !IsScalar();}

	[[nodiscard]] constexpr bool IsPacked() const
	{
		return (value == Vec11f11f10f ||
			(value >= FirstOfPackedInt && value < EndOfPackedInt) ||
			(value >= FirstOfPackedNorm && value < EndOfPackedNorm));
	}

	[[nodiscard]] constexpr bool IsNormalized() const
	{
		return (value >= FirstOfNormalized && value < EndOfNormalized) ||
			(value >= FirstOfPackedNorm && value < EndOfPackedNorm);
	}

	[[nodiscard]] constexpr bool IsInteger() const
	{
		return (value >= FirstOfInteger && value < EndOfInteger) ||
			(value >= FirstOfPackedInt && value < EndOfPackedInt);
	}

	//Возвращает размерность вектора или 1 если это скаляр. Для матриц не работает
	byte Dimensions() const;

	//Возвращает тип элемента вектора или матрицы. Если тип скалярный, то он же и возвращается
	ValueType ToScalarType() const;

	//Переводит все нормализованные типы в float, упакованные и меньшие типы в основные типы (float, double, int, unsigned, bool)
	ValueType ToShaderType() const;

	Intra::StringView ToStringGLSL() const;
	Intra::StringView ToString() const;
	static ValueType FromString(Intra::StringView str);
	static ValueType FromStringGLSL(Intra::StringView str);

	//TODO: Before type list redesign this header added ~22 ms to compilation (84 ms including headers, one of them - TypeList 5 ms)
	//Measure again
	typedef TList<void,

		//Vectors / scalars
		float, Intra::Vec2, Intra::Vec3, Intra::Vec4,

		double, Intra::DVec2, Intra::DVec3, Intra::DVec4,
		Intra::HalfFloat, Intra::Vector2<Intra::HalfFloat>, Intra::Vector3<Intra::HalfFloat>, Intra::Vector4<Intra::HalfFloat>,

		decltype(null),

		int, Intra::IVec2, Intra::IVec3, Intra::IVec4,
		unsigned, Intra::UVec2, Intra::UVec3, Intra::UVec4,
		short, Intra::SVec2, Intra::SVec3, Intra::SVec4,
		uint16, Intra::USVec2, Intra::USVec3, Intra::USVec4,
		int8, Intra::SBVec2, Intra::SBVec3, Intra::SBVec4,
		byte, Intra::UBVec2, Intra::UBVec3, Intra::UBVec4,
		bool, Intra::BVec2, Intra::BVec3, Intra::BVec4,

		decltype(null), decltype(null), decltype(null),

		Intra::Norm8s, Vector2<Intra::Norm8s>, Vector3<Intra::Norm8s>, Vector4<Intra::Norm8s>,
		Intra::Norm16s, Vector2<Intra::Norm16s>, Vector3<Intra::Norm16s>, Vector4<Intra::Norm16s>,
		Intra::Norm32s, Vector2<Intra::Norm32s>, Vector3<Intra::Norm32s>, Vector4<Intra::Norm32s>,
		Intra::SNorm8s, Vector2<Intra::SNorm8s>, Vector3<Intra::SNorm8s>, Vector4<Intra::SNorm8s>,
		Intra::SNorm16s, Vector2<Intra::SNorm16s>, Vector3<Intra::SNorm16s>, Vector4<Intra::SNorm16s>,
		decltype(null), decltype(null), decltype(null), decltype(null),
		//SNorm24s, S24Vec2, S24Vec3, S24Vec4,
		Intra::SNorm32s, Vector2<Intra::SNorm32s>, Vector3<Intra::SNorm32s>, Vector4<Intra::SNorm32s>,

		decltype(null), decltype(null), decltype(null), decltype(null), decltype(null), decltype(null),
		decltype(null), decltype(null), decltype(null), decltype(null),

		//Matrices
		/*Mat2*/decltype(null), decltype(null), decltype(null), decltype(null), Intra::Mat3, decltype(null), decltype(null), decltype(null), Intra::Mat4,

		/*DMat2*/decltype(null), decltype(null), decltype(null), decltype(null), Intra::DMat3, decltype(null), decltype(null), decltype(null), Intra::DMat4,

		char, Intra::String, Intra::StringView, decltype(null), decltype(null)> ValueTypeList;

	template<typename T> [[nodiscard]] static constexpr ValueType Of()
	{
		enum {FoundPos = TListFind<T, ValueTypeList>};
		return FoundPos == ValueTypeList::Length? ValueType::End: FoundPos;
	}
};
INTRA_END
