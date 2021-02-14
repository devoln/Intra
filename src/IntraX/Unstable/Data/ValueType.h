#pragma once

#include "Intra/Core.h"

#include "Intra/Range/StringView.h"

#include "IntraX/Container/ForwardDecls.h"

#include "Intra/Math/Math.h"
#include "IntraX/Math/Matrix3.h"
#include "IntraX/Math/Matrix4.h"
#include "IntraX/Math/Vector2.h"
#include "IntraX/Math/Vector3.h"
#include "IntraX/Math/Vector4.h"
#include "Intra/Math/Fixed.h"
#include "Intra/Math/HalfFloat.h"

#include "Intra/TList.h"

namespace Intra { INTRA_BEGIN
struct ValueType
{
	enum I: byte
	{
		Void = 0,

		Float,  FVec2, FVec3, FVec4,

		Double, DVec2, DVec3, DVec4,
		Half,   HVec2, HVec3, HVec4,

		Vec11f11f10f,

		Int,    IVec2,  IVec3,  IVec4,
		UInt,   UVec2,  UVec3,  UVec4,
		Short,  I16Vec2,  I16Vec3,  I16Vec4,
		UShort, U16Vec2, U16Vec3, U16Vec4,
		SByte,  I8Vec2, I8Vec3, I8Vec4,
		Byte,   U8Vec2, U8Vec3, U8Vec4,
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
		
		FMat2, FMat2x3, FMat2x4, FMat3x2, FMat3, FMat3x4, FMat4x2, FMat4x3, FMat4,

		DMat2, DMat2x3, DMat2x4, DMat3x2, DMat3, DMat3x4, DMat4x2, DMat4x3, DMat4,

		Char,
		String, StringView, StructureInstance, StructureType,

		First = Float, FirstOfVectors=Float, FirstOfFloat = FirstOfVectors, EndOfFloat=HVec4+1,
		FirstOfPackedFloat = Vec11f11f10f, EndOfPackedFloat = Vec11f11f10f+1,
		FirstOfInteger = Int, EndOfInteger = U8Vec4+1,
		FirstOfPackedInt = UVec4444, EndOfPackedInt = UVec9995+1,
		FirstOfNormalized = Norm8, EndOfNormalized = S32Vec4+1,
		FirstOfPackedNorm = NVec233, EndOfPackedNorm = N5Vec3+1, EndOfVectors = EndOfPackedNorm,
		FirstOfMatrices = FMat2, EndOfMatrices = Char, EndOfPod = String, End = StructureType+1
	};

	I Value = Void;

	explicit constexpr ValueType(unsigned vt): Value(I(vt)) {}
	constexpr ValueType(I vt): Value(vt) {}
	ValueType() = default;
	constexpr ValueType(const ValueType& rhs): Value(rhs.Value) {}

	[[nodiscard]] constexpr bool operator bool() const {return Value != Void;}
	[[nodiscard]] constexpr bool operator==(byte rhs) const {return Value == rhs;}
	[[nodiscard]] constexpr bool operator!=(byte rhs) const {return !operator==(rhs);}
	[[nodiscard]] constexpr bool operator==(const ValueType& rhs) const = default;

	uint16 Size() const;

	[[nodiscard]] constexpr bool IsScalar() const noexcept
	{
		return (Value == Float || Value == Double || Value == Half) ||
			(Value >= FirstOfInteger && Value < EndOfInteger && (Value - FirstOfInteger) % 4 == 0) ||
			(Value >= FirstOfNormalized && Value < EndOfNormalized && (Value - FirstOfNormalized) % 4 == 0);
	}

	[[nodiscard]] constexpr bool IsPod() const noexcept {return Value < EndOfPod;}

	[[nodiscard]] constexpr bool IsValid() const noexcept {return Value != Void && Value < End;}
	[[nodiscard]] constexpr bool IsMatrix() const noexcept {return Value >= FirstOfMatrices && Value < EndOfMatrices;}
	[[nodiscard]] constexpr bool IsVector() const noexcept {return Value >= FirstOfVectors && Value < EndOfVectors && !IsScalar();}

	[[nodiscard]] constexpr bool IsPacked() const
	{
		return (Value == Vec11f11f10f ||
			(Value >= FirstOfPackedInt && Value < EndOfPackedInt) ||
			(Value >= FirstOfPackedNorm && Value < EndOfPackedNorm));
	}

	[[nodiscard]] constexpr bool IsNormalized() const
	{
		return (Value >= FirstOfNormalized && Value < EndOfNormalized) ||
			(Value >= FirstOfPackedNorm && Value < EndOfPackedNorm);
	}

	[[nodiscard]] constexpr bool IsInteger() const
	{
		return (Value >= FirstOfInteger && Value < EndOfInteger) ||
			(Value >= FirstOfPackedInt && Value < EndOfPackedInt);
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
	using ValueTypeList = TList<void,

		//Vectors / scalars
		float, Intra::Vec2, Intra::Vec3, Intra::Vec4,

		double, Intra::DVec2, Intra::DVec3, Intra::DVec4,
		Intra::HalfFloat, Intra::Vector2<Intra::HalfFloat>, Intra::Vector3<Intra::HalfFloat>, Intra::Vector4<Intra::HalfFloat>,

		TUndefined,

		int, Intra::IVec2, Intra::IVec3, Intra::IVec4,
		unsigned, Intra::UVec2, Intra::UVec3, Intra::UVec4,
		short, Intra::I16Vec2, Intra::I16Vec3, Intra::I16Vec4,
		uint16, Intra::U16Vec2, Intra::U16Vec3, Intra::U16Vec4,
		int8, Intra::I8Vec2, Intra::I8Vec3, Intra::I8Vec4,
		byte, Intra::U8Vec2, Intra::U8Vec3, Intra::U8Vec4,
		bool, Intra::BVec2, Intra::BVec3, Intra::BVec4,

		TUndefined, TUndefined, TUndefined,

		Intra::Norm8s, Vector2<Intra::Norm8s>, Vector3<Intra::Norm8s>, Vector4<Intra::Norm8s>,
		Intra::Norm16s, Vector2<Intra::Norm16s>, Vector3<Intra::Norm16s>, Vector4<Intra::Norm16s>,
		Intra::Norm32s, Vector2<Intra::Norm32s>, Vector3<Intra::Norm32s>, Vector4<Intra::Norm32s>,
		Intra::SNorm8s, Vector2<Intra::SNorm8s>, Vector3<Intra::SNorm8s>, Vector4<Intra::SNorm8s>,
		Intra::SNorm16s, Vector2<Intra::SNorm16s>, Vector3<Intra::SNorm16s>, Vector4<Intra::SNorm16s>,
		TUndefined, TUndefined, TUndefined, TUndefined,
		//SNorm24s, S24Vec2, S24Vec3, S24Vec4,
		Intra::SNorm32s, Vector2<Intra::SNorm32s>, Vector3<Intra::SNorm32s>, Vector4<Intra::SNorm32s>,

		TUndefined, TUndefined, TUndefined, TUndefined, TUndefined, TUndefined,
		TUndefined, TUndefined, TUndefined, TUndefined,

		//Matrices
		/*Mat2*/TUndefined, TUndefined, TUndefined, TUndefined, Intra::Mat3, TUndefined, TUndefined, TUndefined, Intra::Mat4,

		/*DMat2*/TUndefined, TUndefined, TUndefined, TUndefined, Intra::DMat3, TUndefined, TUndefined, TUndefined, Intra::DMat4,

		char, Intra::String, Intra::StringView, TUndefined, TUndefined>;

	template<typename T> [[nodiscard]] static constexpr ValueType Of()
	{
		enum {FoundPos = TListFind<T, ValueTypeList>};
		return FoundPos == ValueTypeList::Length? ValueType::End: FoundPos;
	}
};
} INTRA_END
