#include "ValueType.h"

#include "Intra/Range/StringView.h"

#include "IntraX/Math/Vector2.h"
#include "IntraX/Math/Vector3.h"
#include "IntraX/Math/Vector4.h"
#include "IntraX/Math/Matrix3.h"
#include "IntraX/Math/Matrix4.h"
#include "IntraX/Container/Associative/HashMap.h"


namespace Intra { INTRA_BEGIN
uint16 ValueType::Size() const
{
	INTRA_PRECONDITION(Value < End);
	namespace T = Intra;
	static constexpr byte scalarVecSizeTable[] = {0,
		sizeof(float),  sizeof(T::Vec2),  sizeof(T::Vec3),  sizeof(T::Vec4),
		sizeof(double), sizeof(T::DVec2), sizeof(T::DVec3), sizeof(T::DVec4),
		2, 4, 6, 8,
		4,

		sizeof(int32),  sizeof(T::IVec2),   sizeof(T::IVec3),   sizeof(T::IVec4),
		sizeof(uint32), sizeof(T::UVec2),   sizeof(T::UVec3),   sizeof(T::UVec4),
		sizeof(int16),  sizeof(T::I16Vec2), sizeof(T::I16Vec3), sizeof(T::I16Vec4),
		sizeof(uint16), sizeof(T::U16Vec2), sizeof(T::U16Vec3), sizeof(T::U16Vec4),
		sizeof(int8),   sizeof(T::I8Vec2),  sizeof(T::I8Vec3),  sizeof(T::I8Vec4),
		sizeof(uint8),  sizeof(T::U8Vec2),  sizeof(T::U8Vec3),  sizeof(T::U8Vec4),
		sizeof(bool),   sizeof(T::BVec2),   sizeof(T::BVec3),   sizeof(T::BVec4),

		2, 4, 4,

		sizeof(byte),   sizeof(T::U8Vec2),  sizeof(T::U8Vec3),  sizeof(T::U8Vec4),
		sizeof(uint16), sizeof(T::U16Vec2), sizeof(T::U16Vec3), sizeof(T::U16Vec4),
		sizeof(uint32), sizeof(T::UVec2),   sizeof(T::UVec3),   sizeof(T::UVec4),
		sizeof(int8),   sizeof(T::I8Vec2),  sizeof(T::I8Vec3),  sizeof(T::I8Vec4),
		sizeof(int16),  sizeof(T::I16Vec2), sizeof(T::I16Vec3), sizeof(T::I16Vec4),
		3, 6, 9, 12,
		sizeof(int),    sizeof(T::IVec2),  sizeof(T::IVec3),  sizeof(T::IVec4),

		1, 2, 2, 2, 4, 4,
		1, 2, 4, 2
	};
	static_assert(Length(scalarVecSizeTable) == EndOfVectors);

	static constexpr uint16 matSizeTable[] = {
		16, 24, 32, 24, sizeof(Mat3), 48, 32, 48, sizeof(Mat4),
		32, 48, 64,  48, 72, 96,  64, 96, 256
	};
	static_assert(Length(matSizeTable) == EndOfMatrices - FirstOfMatrices);

	if(Value < EndOfVectors) return scalarVecSizeTable[Value];
	if(Value < EndOfMatrices) return matSizeTable[Value - FirstOfMatrices];
	if(Value == ValueType::Char) return sizeof(char);
	return INTRA_FATAL_ERROR("Unknown type of this ValueType!"), uint16(0);
}

//Возвращает размерность вектора или 1 если это скаляр. Для матриц не работает
byte ValueType::Dimensions() const
{
	if(Value >= FirstOfFloat && Value < EndOfFloat) return byte( (Value - FirstOfFloat)%4 + 1 );
	if(Value >= FirstOfInteger && Value < EndOfInteger) return byte( (Value - FirstOfInteger)%4 + 1 );
	if(Value >= FirstOfNormalized && Value < EndOfNormalized) return byte( (Value - FirstOfNormalized)%4 + 1 );
	if(Value == Vec11f11f10f || Value == NVec332 || Value == NVec565) return 3;
	if(Value == NVec5551 || Value == NVec4444 || Value == Vec10n10n10n2n ||
		Value == Vec10s10s10s2s || Value == Vec10u10u10u2u || Value == UVec9995) return 4;
	return 0;
}

ValueType ValueType::ToScalarType() const
{
	if(Value >= FirstOfFloat      && Value < EndOfFloat)      return ValueType( unsigned(Value-(Value - FirstOfFloat)%4) );
	if(Value >= FirstOfInteger    && Value < EndOfInteger)    return ValueType( unsigned(Value-(Value - FirstOfInteger)%4) );
	if(Value >= FirstOfNormalized && Value < EndOfNormalized) return ValueType( unsigned(Value-(Value - FirstOfNormalized)%4) );
	if(Value >= FMat2 && Value <= FMat4) return Float;
	if(Value >= DMat2 && Value <= DMat4) return Double;
	return Void;
}

ValueType ValueType::ToShaderType() const
{
	if(Value >= Half && Value <= HVec4) return ValueType(Float+unsigned(Value-Half));
	if(Value == Vec11f11f10f) return Vec3;
	if(Value <= UVec4) return *this;
	if(Value >= FMat2 && Value <= DMat4) return *this;
	if(Value >= Norm8 && Value <= S32Vec4) return ValueType(Float+(Dimensions() - 1u));
	if(Value >= UVec4444 && Value <= UVec9995) return UVec4;

	static const ValueType::I table[] = {
		Vec3, Vec3, Vec4, Vec4,
		Vec4, Vec4,
		Vec3, Vec4, Vec3, Vec3
	};
	if(Value >= NVec233 && Value <= N5Vec3) return table[Value - NVec233];
	return Void;
}

constexpr const char* valueTypeGLSLNameTable[] = {
	"void",

	"float",  "vec2",  "vec3",  "vec4",
	"double", "dvec2", "dvec3", "dvec4",
	nullptr, nullptr, nullptr, nullptr,

	nullptr,

	"int",  "ivec2", "ivec3", "ivec4",
	"uint", "uvec2", "uvec3", "uvec4",
	nullptr,  nullptr, nullptr, nullptr,
	nullptr,  nullptr, nullptr, nullptr,
	nullptr,  nullptr, nullptr, nullptr,
	nullptr,  nullptr, nullptr, nullptr,
	"bool", "bvec2", "bvec3", "bvec4",

	nullptr, nullptr, nullptr,

	nullptr, nullptr, nullptr, nullptr,
	nullptr, nullptr, nullptr, nullptr,
	nullptr, nullptr, nullptr, nullptr,
	nullptr, nullptr, nullptr, nullptr,
	nullptr, nullptr, nullptr, nullptr,
	nullptr, nullptr, nullptr, nullptr,
	nullptr, nullptr, nullptr, nullptr,

	nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
	nullptr, nullptr, nullptr, nullptr,

	nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
	nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,

	nullptr, nullptr, nullptr, nullptr, "struct"
};
static_assert(Length(valueTypeGLSLNameTable) == ValueType::End);

StringView ValueType::ToStringGLSL() const
{
	return Value > End? nullptr: Intra::StringView(valueTypeGLSLNameTable[Value]);
}

constexpr const char* valueTypeNameTable[] = {
	"Void",

	//Векторы\скаляры
	"Float",  "Vec2", "Vec3", "Vec4",

	"Double", "DVec2", "DVec3", "DVec4",
	"Half",   "HVec2", "HVec3", "HVec4",

	"Bool", "BVec2", "BVec3", "BVec4",

	"Vec11f11f10f",

	"Int",    "IVec2",  "IVec3",  "IVec4",
	"UInt",   "UVec2",  "UVec3",  "UVec4",
	"Short",  "I16Vec2",  "I16Vec3",  "I16Vec4",
	"UShort", "U16Vec2", "U16Vec3", "U16Vec4",
	"SByte",  "I8Vec2", "I8Vec3", "I8Vec4",
	"Byte",   "U8Vec2", "U8Vec3", "U8Vec4",

	"UVec4444", "Vec10u10u10u2u", "UVec9995",

	"Norm8",   "N8Vec2",  "N8Vec3",  "N8Vec4",
	"Norm16",  "N16Vec2", "N16Vec3", "N16Vec4",
	"Norm32",  "N32Vec2", "N32Vec3", "N32Vec4",
	"SNorm8",  "S8Vec2",  "S8Vec3",  "S8Vec4",
	"SNorm16", "S16Vec2", "S16Vec3", "S16Vec4",
	"SNorm24", "S24Vec2", "S24Vec3", "S24Vec4",
	"SNorm32", "S32Vec2", "S32Vec3", "S32Vec4",

	"NVec233", "NVec565", "NVec1555", "NVec4444", "Vec10n10n10n2n", "Vec10s10s10s2s",
	"NVec332", "NVec5551", "N10Vec3", "N5Vec3",

	//Матрицы
	"FMat2", "FMat2x3", "FMat2x4", "FMat3x2", "FMat3", "FMat3x4", "FMat4x2", "FMat4x3", "FMat4",

	"DMat2", "DMat2x3", "DMat2x4", "DMat3x2", "DMat3", "DMat3x4", "DMat4x2", "DMat4x3", "DMat4",

	"Char", "String", "StringView", "StructureType", "StructureInstance"
};
static_assert(Length(valueTypeNameTable) == ValueType::End);

StringView ValueType::ToString() const
{
	return Value > End? nullptr: Intra::StringView(valueTypeNameTable[Value]);
}

ValueType ValueType::FromString(Intra::StringView str)
{
	INTRA_IGNORE_WARN_GLOBAL_CONSTRUCTION
	static HashMap<Intra::StringView, ValueType> dictionary;
	if(dictionary.Empty())
	{
		for(ValueType i = ValueType::Void; i.Value < ValueType::End; i = I(i.Value+1))
			dictionary.InsertNew(i.ToString(), i);
	}
	return dictionary.Get(str, ValueType::Void);
}

ValueType ValueType::FromStringGLSL(Intra::StringView str)
{
	//Lazy initialization of type -> enum dictionary
	static HashMap<Intra::StringView, ValueType::I> dictionary;
	if(dictionary.Empty())
	{
		for(I i = ValueType::Void; i<ValueType::End; i = ValueType::I(i+1))
			dictionary.InsertNew(ValueType(i).ToStringGLSL(), i);
	}
	return dictionary.Get(str, ValueType::Void);
}
} INTRA_END
