#pragma once

#include "MathEx.h"

#include "Vector.h"
#include "Matrix.h"

#include "Data/ValueType.h"


#if INTRA_DISABLED
template<typename T> struct MathTypeTraits;
#define DEFINE_TYPE_TRAITS(type, vt) template<> struct MathTypeTraits<type>\
{\
	static const auto ValueType = ValueType::vt;\
	static const char* TypeName() {return # type ;}\
	static const char* ValueTypeName() {return # vt ;}\
	template<typename OBJ> using Member = type OBJ::*;\
	template<typename OBJ> using ArrayMember = Array<type> OBJ::*;\
};

#define DEFINE_MATH_TYPE_TRAITS(type, vt) template<> struct MathTypeTraits<Math::type>\
{\
	static const auto ValueType = ValueType::vt;\
	static const char* TypeName() {return # type ;}\
	static const char* ValueTypeName() {return # vt ;}\
	template<typename OBJ> using Member = Math::type OBJ::*;\
	template<typename OBJ> using ArrayMember = Array<Math::type> OBJ::*;\
};

template<> struct MathTypeTraits<void>
{
	static const auto ValueType = ValueType::Void;
	static const char* TypeName() {return "void";}
	static const char* ValueTypeName() {return "Void";}
};

DEFINE_TYPE_TRAITS(float, Float)
DEFINE_MATH_TYPE_TRAITS(Vec2, Vec2)
DEFINE_MATH_TYPE_TRAITS(Vec3, Vec3)
DEFINE_MATH_TYPE_TRAITS(Vec4, Vec4)

DEFINE_TYPE_TRAITS(double, Double)
DEFINE_MATH_TYPE_TRAITS(DVec2, DVec2)
DEFINE_MATH_TYPE_TRAITS(DVec3, DVec3)
DEFINE_MATH_TYPE_TRAITS(DVec4, DVec4)

DEFINE_TYPE_TRAITS(int, Int)
DEFINE_MATH_TYPE_TRAITS(IVec2, IVec2)
DEFINE_MATH_TYPE_TRAITS(IVec3, IVec3)
DEFINE_MATH_TYPE_TRAITS(IVec4, IVec4)

DEFINE_TYPE_TRAITS(uint, UInt)
DEFINE_MATH_TYPE_TRAITS(UVec2, UVec2)
DEFINE_MATH_TYPE_TRAITS(UVec3, UVec3)
DEFINE_MATH_TYPE_TRAITS(UVec4, UVec4)

DEFINE_TYPE_TRAITS(short, Short)
DEFINE_MATH_TYPE_TRAITS(SVec2, SVec2)
DEFINE_MATH_TYPE_TRAITS(SVec3, SVec3)
DEFINE_MATH_TYPE_TRAITS(SVec4, SVec4)

DEFINE_TYPE_TRAITS(ushort, UShort)
DEFINE_MATH_TYPE_TRAITS(USVec2, USVec2)
DEFINE_MATH_TYPE_TRAITS(USVec3, USVec3)
DEFINE_MATH_TYPE_TRAITS(USVec4, USVec4)

DEFINE_TYPE_TRAITS(sbyte, SByte)
DEFINE_MATH_TYPE_TRAITS(SBVec2, SBVec2)
DEFINE_MATH_TYPE_TRAITS(SBVec3, SBVec3)
DEFINE_MATH_TYPE_TRAITS(SBVec4, SBVec4)

DEFINE_TYPE_TRAITS(byte, Byte)
DEFINE_MATH_TYPE_TRAITS(UBVec2, UBVec2)
DEFINE_MATH_TYPE_TRAITS(UBVec3, UBVec3)
DEFINE_MATH_TYPE_TRAITS(UBVec4, UBVec4)

DEFINE_TYPE_TRAITS(bool, Bool)
DEFINE_MATH_TYPE_TRAITS(BVec2, BVec2)
DEFINE_MATH_TYPE_TRAITS(BVec3, BVec3)
DEFINE_MATH_TYPE_TRAITS(BVec4, BVec4)

//DEFINE_MATH_TYPE_TRAITS(mat2, Mat2)
//DEFINE_MATH_TYPE_TRAITS(mat2x3, Mat2x3)
//DEFINE_MATH_TYPE_TRAITS(mat2x4, Mat2x4)
//DEFINE_MATH_TYPE_TRAITS(mat3x2, Mat3x2)
DEFINE_MATH_TYPE_TRAITS(Mat3, Mat3)
//DEFINE_MATH_TYPE_TRAITS(mat3x4, Mat3x4)
//DEFINE_MATH_TYPE_TRAITS(mat4x2, Mat4x2)
//DEFINE_MATH_TYPE_TRAITS(mat4x3, Mat4x3)
DEFINE_MATH_TYPE_TRAITS(Mat4, Mat4)

//DEFINE_MATH_TYPE_TRAITS(dmat2, DMat2)
//DEFINE_MATH_TYPE_TRAITS(dmat2x3, DMat2x3)
//DEFINE_MATH_TYPE_TRAITS(dmat2x4, DMat2x4)
//DEFINE_MATH_TYPE_TRAITS(dmat3x2, DMat3x2)
DEFINE_MATH_TYPE_TRAITS(dmat3, Mat3)
//DEFINE_MATH_TYPE_TRAITS(dmat3x4, DMat3x4)
//DEFINE_MATH_TYPE_TRAITS(dmat4x2, DMat4x2)
//DEFINE_MATH_TYPE_TRAITS(dmat4x3, DMat4x3)
DEFINE_MATH_TYPE_TRAITS(dmat4, DMat4)

#undef DEFINE_TYPE_TRAITS
#undef DEFINE_MATH_TYPE_TRAITS
#endif
