#include "ValueType.h"

#include "Intra/Range/StringView.h"

#include "Extra/Math/Vector2.h"
#include "Extra/Math/Vector3.h"
#include "Extra/Math/Vector4.h"
#include "Extra/Math/Matrix3.h"
#include "Extra/Math/Matrix4.h"
#include "Extra/Container/Associative/HashMap.h"


INTRA_BEGIN
uint16 ValueType::Size() const
{
	INTRA_PRECONDITION(value < End);
	namespace T = Intra;
	static constexpr byte scalarVecSizeTable[] = {0,
		sizeof(float),  sizeof(T::Vec2),  sizeof(T::Vec3),  sizeof(T::Vec4),
		sizeof(double), sizeof(T::DVec2), sizeof(T::DVec3), sizeof(T::DVec4),
		2, 4, 6, 8,
		4,

		sizeof(int),    sizeof(T::IVec2),  sizeof(T::IVec3),  sizeof(T::IVec4),
		sizeof(unsigned),   sizeof(T::UVec2),  sizeof(T::UVec3),  sizeof(T::UVec4),
		sizeof(short),  sizeof(T::SVec2),  sizeof(T::SVec3),  sizeof(T::SVec4),
		sizeof(uint16), sizeof(T::USVec2), sizeof(T::USVec3), sizeof(T::USVec4),
		sizeof(int8),  sizeof(T::SBVec2), sizeof(T::SBVec3), sizeof(T::SBVec4),
		sizeof(byte),   sizeof(T::UBVec2), sizeof(T::UBVec3), sizeof(T::UBVec4),
		sizeof(bool),   sizeof(T::BVec2),  sizeof(T::BVec3),  sizeof(T::BVec4),

		2,4,4,

		sizeof(byte),   sizeof(T::UBVec2), sizeof(T::UBVec3), sizeof(T::UBVec4),
		sizeof(uint16), sizeof(T::USVec2), sizeof(T::USVec3), sizeof(T::USVec4),
		sizeof(unsigned),   sizeof(T::UVec2),  sizeof(T::UVec3),  sizeof(T::UVec4),
		sizeof(int8),  sizeof(T::SBVec2), sizeof(T::SBVec3), sizeof(T::SBVec4),
		sizeof(short),  sizeof(T::SVec2),  sizeof(T::SVec3),  sizeof(T::SVec4),
		3,6,9,12,
		sizeof(int),    sizeof(T::IVec2),  sizeof(T::IVec3),  sizeof(T::IVec4),

		1,2,2,2,4,4,
		1,2,4,2
	};
	INTRA_CHECK_TABLE_SIZE(scalarVecSizeTable, EndOfVectors);

	static constexpr uint16 matSizeTable[]={
		16, 24, 32, 24, sizeof(Mat3), 48, 32, 48, sizeof(Mat4),
		32,48,64, 48,72,96, 64,96,256
	};
	INTRA_CHECK_TABLE_SIZE(matSizeTable, EndOfMatrices-FirstOfMatrices);

	if(value < EndOfVectors) return scalarVecSizeTable[value];
	if(value < EndOfMatrices) return matSizeTable[value - FirstOfMatrices];
	if(value == ValueType::Char) return sizeof(char);
	return INTRA_FATAL_ERROR("Unknown type of this ValueType!"), uint16(0);
}

//Возвращает размерность вектора или 1 если это скаляр. Для матриц не работает
byte ValueType::Dimensions() const
{
	if(value >= FirstOfFloat && value < EndOfFloat) return byte( (value - FirstOfFloat)%4 + 1 );
	if(value >= FirstOfInteger && value < EndOfInteger) return byte( (value - FirstOfInteger)%4 + 1 );
	if(value >= FirstOfNormalized && value < EndOfNormalized) return byte( (value - FirstOfNormalized)%4 + 1 );
	if(value == Vec11f11f10f || value == NVec332 || value == NVec565) return 3;
	if(value == NVec5551 || value == NVec4444 || value == Vec10n10n10n2n ||
		value == Vec10s10s10s2s || value == Vec10u10u10u2u || value == UVec9995) return 4;
	return 0;
}

ValueType ValueType::ToScalarType() const
{
	if(value >= FirstOfFloat      && value < EndOfFloat)      return ValueType( unsigned(value-(value - FirstOfFloat)%4) );
	if(value >= FirstOfInteger    && value < EndOfInteger)    return ValueType( unsigned(value-(value - FirstOfInteger)%4) );
	if(value >= FirstOfNormalized && value < EndOfNormalized) return ValueType( unsigned(value-(value - FirstOfNormalized)%4) );
	if(value >= FMat2 && value <= FMat4) return Float;
	if(value >= DMat2 && value <= DMat4) return Double;
	return Void;
}

ValueType ValueType::ToShaderType() const
{
	if(value >= Half && value <= HVec4) return ValueType(Float+unsigned(value-Half));
	if(value == Vec11f11f10f) return Vec3;
	if(value <= UVec4) return *this;
	if(value >= FMat2 && value <= DMat4) return *this;
	if(value >= Norm8 && value <= S32Vec4) return ValueType(Float+(Dimensions() - 1u));
	if(value >= UVec4444 && value <= UVec9995) return UVec4;

	static const ValueType::I table[] = {
		Vec3, Vec3, Vec4, Vec4,
		Vec4, Vec4,
		Vec3, Vec4, Vec3, Vec3
	};
	if(value >= NVec233 && value <= N5Vec3) return table[value - NVec233];
	return Void;
}

static const char* const valueTypeGLSLNameTable[] = {
	"void",

	"float",  "vec2",  "vec3",  "vec4",
	"double", "dvec2", "dvec3", "dvec4",
	null, null, null, null,

	null,

	"int",  "ivec2", "ivec3", "ivec4",
	"unsigned", "uvec2", "uvec3", "uvec4",
	null,  null, null, null,
	null,  null, null, null,
	null,  null, null, null,
	null,  null, null, null,
	"bool", "bvec2", "bvec3", "bvec4",

	null, null, null,

	null, null, null, null,
	null, null, null, null,
	null, null, null, null,
	null, null, null, null,
	null, null, null, null,
	null, null, null, null,
	null, null, null, null,

	null, null, null, null, null, null,
	null, null, null, null,

	null, null, null, null, null, null, null, null, null,
	null, null, null, null, null, null, null, null, null,

	null, null, null, null, "struct"
};
INTRA_CHECK_TABLE_SIZE(valueTypeGLSLNameTable, ValueType::End);

StringView ValueType::ToStringGLSL() const
{
	return value > End? null: Intra::StringView(valueTypeGLSLNameTable[value]);
}

static const char* const valueTypeNameTable[] = {
	"Void",

	//Векторы\скаляры
	"Float",  "Vec2", "Vec3", "Vec4",

	"Double", "DVec2", "DVec3", "DVec4",
	"Half",   "HVec2", "HVec3", "HVec4",

	"Bool", "BVec2", "BVec3", "BVec4",

	"Vec11f11f10f",

	"Int",    "IVec2",  "IVec3",  "IVec4",
	"UInt",   "UVec2",  "UVec3",  "UVec4",
	"Short",  "SVec2",  "SVec3",  "SVec4",
	"UShort", "USVec2", "USVec3", "USVec4",
	"SByte",  "SBVec2", "SBVec3", "SBVec4",
	"Byte",   "UBVec2", "UBVec3", "UBVec4",

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
	//Mat2=FMat2, Mat2x3=FMat2x3, Mat2x4=FMat2x4, Mat3x2=FMat3x2, Mat3=FMat3, Mat3x3=Mat3, FMat3x3=Mat3, Mat3x4=FMat3x4, Mat4x2=FMat4x2, Mat4x3=FMat4x3, Mat4=FMat4, FMat4x4=Mat4, Mat4x4=Mat4,

	"DMat2", "DMat2x3", "DMat2x4", "DMat3x2", "DMat3", "DMat3x4", "DMat4x2", "DMat4x3", "DMat4",

	"Char", "String", "StringView", "StructureType", "StructureInstance"
};
INTRA_CHECK_TABLE_SIZE(valueTypeNameTable, ValueType::End);

StringView ValueType::ToString() const
{
	return value > End? null: Intra::StringView(valueTypeNameTable[value]);
}

ValueType ValueType::FromString(Intra::StringView str)
{
	INTRA_IGNORE_WARNING_GLOBAL_CONSTRUCTION
	static HashMap<Intra::StringView, ValueType> dictionary;
	if(dictionary == null)
	{
		for(ValueType i = ValueType::Void; i.value < ValueType::End; i = I(i.value+1))
			dictionary.InsertNew(i.ToString(), i);
	}
	return dictionary.Get(str, ValueType::Void);
}

ValueType ValueType::FromStringGLSL(Intra::StringView str)
{
	//Lazy initialization of type -> enum dictionary
	static HashMap<Intra::StringView, ValueType::I> dictionary;
	if(dictionary == null)
	{
		for(I i = ValueType::Void; i<ValueType::End; i = ValueType::I(i+1))
			dictionary.InsertNew(ValueType(i).ToStringGLSL(), i);
	}
	return dictionary.Get(str, ValueType::Void);
}
INTRA_END
