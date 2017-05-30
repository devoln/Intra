#include "Data/Variable.h"
#include "Cpp/Warnings.h"

namespace Intra { namespace Data {

using namespace Math;

INTRA_DISABLE_REDUNDANT_WARNINGS

void Variable::ConvertType(Variable& dst, ValueType srcType, ValueType dstType) const
{
	if(srcType==dstType)
	{
		C::memcpy(&dst, this, dstType.Size());
		return;
	}

	Variable& r = dst;
	Variable var;
	DVec4& d4 = var.AsDVec4;
	d4 = DVec4(0);

	if(srcType.ToScalarType()==ValueType::Double) d4 = AsDVec4;
	else switch(srcType.value)
	{
	case ValueType::Void: break;

	case ValueType::Bool: d4.x = double(AsBool); break;
	case ValueType::BVec2: d4.xy = DVec2(AsBVec2); break;
	case ValueType::BVec3: d4.xyz = DVec3(AsBVec3); break;
	case ValueType::BVec4: d4 = DVec4(AsBVec4); break;

	case ValueType::Norm8: d4.x = AsByte/255.0; break;
	case ValueType::SNorm8: d4.x = AsSByte/127.0; break;
	case ValueType::Norm16: d4.x = AsUShort/65535.0; break;
	case ValueType::SNorm16: d4.x = AsShort/32767.0; break;
	case ValueType::Norm32: d4.x = AsUInt/double(uint_MAX); break;
	case ValueType::SNorm32: d4.x = AsInt/double(int_MAX); break;

	case ValueType::SByte: d4.x = AsByte; break;
	case ValueType::Byte: d4.x = AsSByte; break;
	case ValueType::UShort: d4.x = AsUShort; break;
	case ValueType::Short: d4.x = AsShort; break;
	case ValueType::UInt: d4.x = AsUInt; break;
	case ValueType::Int: d4.x = AsInt; break;

	case ValueType::Half: d4.x = double(AsHalf); break;
	case ValueType::Float: d4.x = AsFloat; break;


	case ValueType::N8Vec2: d4.xy = DVec2(AsUBVec2)/255.0; break;
	case ValueType::S8Vec2: d4.xy = DVec2(AsSBVec2)/127.0; break;
	case ValueType::N16Vec2: d4.xy = DVec2(AsUSVec2)/65535.0; break;
	case ValueType::S16Vec2: d4.xy = DVec2(AsSVec2)/32767.0; break;
	case ValueType::N32Vec2: d4.xy = DVec2(AsUVec2)/double(uint_MAX); break;
	case ValueType::S32Vec2: d4.xy = DVec2(AsIVec2)/double(int_MAX); break;

	case ValueType::UBVec2: d4.xy = DVec2(AsUBVec2); break;
	case ValueType::SBVec2: d4.xy = DVec2(AsSBVec2); break;
	case ValueType::USVec2: d4.xy = DVec2(AsUSVec2); break;
	case ValueType::SVec2: d4.xy = DVec2(AsSVec2); break;
	case ValueType::UVec2: d4.xy = DVec2(AsUVec2); break;
	case ValueType::IVec2: d4.xy = DVec2(AsIVec2); break;

	case ValueType::HVec2: d4.xy = DVec2(AsHVec2); break;
	case ValueType::Vec2: d4.xy = DVec2(AsVec2); break;


	case ValueType::N8Vec3: d4.xyz = DVec3(AsUBVec3)/255.0; break;
	case ValueType::S8Vec3: d4.xyz = DVec3(AsSBVec3)/127.0; break;
	case ValueType::N16Vec3: d4.xyz = DVec3(AsUSVec3)/65535.0; break;
	case ValueType::S16Vec3: d4.xyz = DVec3(AsSVec3)/32767.0; break;
	case ValueType::N32Vec3: d4.xyz = DVec3(AsUVec3)/double(uint_MAX); break;
	case ValueType::S32Vec3: d4.xyz = DVec3(AsIVec3)/double(int_MAX); break;

	case ValueType::UBVec3: d4.xyz = DVec3(AsUBVec3); break;
	case ValueType::SBVec3: d4.xyz = DVec3(AsSBVec3); break;
	case ValueType::USVec3: d4.xyz = DVec3(AsUSVec3); break;
	case ValueType::SVec3: d4.xyz = DVec3(AsSVec3); break;
	case ValueType::UVec3: d4.xyz = DVec3(AsUVec3); break;
	case ValueType::IVec3: d4.xyz = DVec3(AsIVec3); break;

	case ValueType::HVec3: d4.xyz = DVec3(AsHVec3); break;
	case ValueType::Vec3: d4.xyz = DVec3(AsVec3); break;


	case ValueType::N8Vec4: d4 = DVec4(AsUBVec4)/255.0; break;
	case ValueType::S8Vec4: d4 = DVec4(AsSBVec4)/127.0; break;
	case ValueType::N16Vec4: d4 = DVec4(AsUSVec4)/65535.0; break;
	case ValueType::S16Vec4: d4 = DVec4(AsSVec4)/32767.0; break;
	case ValueType::N32Vec4: d4 = DVec4(AsUVec4)/float(uint_MAX); break;
	case ValueType::S32Vec4: d4 = DVec4(AsIVec4)/float(int_MAX); break;

	case ValueType::UBVec4: d4 = DVec4(AsUBVec4); break;
	case ValueType::SBVec4: d4 = DVec4(AsSBVec4); break;
	case ValueType::USVec4: d4 = DVec4(AsUSVec4); break;
	case ValueType::SVec4: d4 = DVec4(AsSVec4); break;
	case ValueType::UVec4: d4 = DVec4(AsUVec4); break;
	case ValueType::IVec4: d4 = DVec4(AsIVec4); break;

	case ValueType::HVec4: d4 = DVec4(AsHVec4); break;
	case ValueType::Vec4: d4 = DVec4(AsVec4); break;


	case ValueType::Mat3:
		INTRA_DEBUG_ASSERT(dstType==ValueType::Mat4);
		r.AsMat4 = Mat4(AsMat3);
		return;

	case ValueType::Mat4:
		INTRA_DEBUG_ASSERT(dstType==ValueType::Mat3);
		r.AsMat3 = Mat3(AsMat4);
		return;

	default:;
	}

	var.ConvertTypeFromDVec4(dst, dstType);
}

void Variable::ConvertTypeFromDVec4(Variable& dst, ValueType dstType) const
{
	if(dstType==ValueType::DVec4)
	{
		C::memcpy(&dst, this, dstType.Size());
		return;
	}

	auto& d4 = AsDVec4;
	auto& r = dst;
	switch(dstType.value)
	{
	case ValueType::Void: break;

	case ValueType::Bool: r.AsBool = d4.x!=0; break;
	case ValueType::BVec2: r.AsBVec2 = {d4.x!=0, d4.y!=0}; break;
	case ValueType::BVec3: r.AsBVec3 = {d4.x!=0, d4.y!=0, d4.z!=0}; break;
	case ValueType::BVec4: r.AsBVec4 = {d4.x!=0, d4.y!=0, d4.z!=0, d4.w!=0}; break;

	case ValueType::Norm8: r.AsByte = byte(d4.x*255.0); break;
	case ValueType::SNorm8: r.AsSByte = sbyte(d4.x*127.0); break;
	case ValueType::Norm16: r.AsUShort = ushort(d4.x*65535.0); break;
	case ValueType::SNorm16: r.AsShort = short(d4.x*32767.0); break;
	case ValueType::Norm32: r.AsUInt = uint(d4.x*uint_MAX); break;
	case ValueType::SNorm32: r.AsInt = int(d4.x*int_MAX); break;

	case ValueType::SByte: r.AsByte = byte(d4.x); break;
	case ValueType::Byte: r.AsSByte = sbyte(d4.x); break;
	case ValueType::UShort: r.AsUShort = ushort(d4.x); break;
	case ValueType::Short: r.AsShort = short(d4.x); break;
	case ValueType::UInt: r.AsUInt = uint(d4.x); break;
	case ValueType::Int: r.AsInt = int(d4.x); break;

	case ValueType::Half: r.AsHalf = HalfFloat(d4.x); break;
	case ValueType::Float: r.AsFloat = float(d4.x); break;
	case ValueType::Double: r.AsDouble = d4.x; break;


	case ValueType::N8Vec2: r.AsUBVec2 = UBVec2(d4.xy*255.0); break;
	case ValueType::S8Vec2: r.AsSBVec2 = SBVec2(d4.xy*127.0); break;
	case ValueType::N16Vec2: r.AsUSVec2 = USVec2(d4.xy*65535.0); break;
	case ValueType::S16Vec2: r.AsSVec2 = SVec2(d4.xy*32767.0); break;
	case ValueType::N32Vec2: r.AsUVec2 = UVec2(d4.xy*uint_MAX); break;
	case ValueType::S32Vec2: r.AsIVec2 = IVec2(d4.xy*int_MAX); break;

	case ValueType::UBVec2: r.AsUBVec2 = UBVec2(d4.xy); break;
	case ValueType::SBVec2: r.AsSBVec2 = SBVec2(d4.xy); break;
	case ValueType::USVec2: r.AsUSVec2 = USVec2(d4.xy); break;
	case ValueType::SVec2: r.AsSVec2 = SVec2(d4.xy); break;
	case ValueType::UVec2: r.AsUVec2 = UVec2(d4.xy); break;
	case ValueType::IVec2: r.AsIVec2 = IVec2(d4.xy); break;

	case ValueType::HVec2: r.AsHVec2 = Vector2<HalfFloat>(d4.xy); break;
	case ValueType::Vec2: r.AsVec2 = Vec2(d4.xy); break;
	case ValueType::DVec2: r.AsDVec2 = d4.xy; break;


	case ValueType::N8Vec3: r.AsUBVec3 = UBVec3(d4.xyz*255.0); break;
	case ValueType::S8Vec3: r.AsSBVec3 = SBVec3(d4.xyz*127.0); break;
	case ValueType::N16Vec3: r.AsUSVec3 = USVec3(d4.xyz*65535.0); break;
	case ValueType::S16Vec3: r.AsSVec3 = SVec3(d4.xyz*32767.0); break;
	case ValueType::N32Vec3: r.AsUVec3 = UVec3(d4.xyz*uint_MAX); break;
	case ValueType::S32Vec3: r.AsIVec3 = IVec3(d4.xyz*int_MAX); break;

	case ValueType::UBVec3: r.AsUBVec3 = UBVec3(d4.xyz); break;
	case ValueType::SBVec3: r.AsSBVec3 = SBVec3(d4.xyz); break;
	case ValueType::USVec3: r.AsUSVec3 = USVec3(d4.xyz); break;
	case ValueType::SVec3: r.AsSVec3 = SVec3(d4.xyz); break;
	case ValueType::UVec3: r.AsUVec3 = UVec3(d4.xyz); break;
	case ValueType::IVec3: r.AsIVec3 = IVec3(d4.xyz); break;

	case ValueType::HVec3: r.AsHVec3 = Vector3<HalfFloat>(d4.xyz); break;
	case ValueType::Vec3: r.AsVec3 = Vec3(d4.xyz); break;
	case ValueType::DVec3: r.AsDVec3 = d4.xyz; break;


	case ValueType::N8Vec4: r.AsUBVec4 = d4*255.0; break;
	case ValueType::S8Vec4: r.AsSBVec4 = d4*127.0; break;
	case ValueType::N16Vec4: r.AsUSVec4 = d4*65535.0; break;
	case ValueType::S16Vec4: r.AsSVec4 = d4*32767.0; break;
	case ValueType::N32Vec4: r.AsUVec4 = d4*uint_MAX; break;
	case ValueType::S32Vec4: r.AsIVec4 = d4*int_MAX; break;

	case ValueType::UBVec4: r.AsUBVec4 = UBVec4(d4); break;
	case ValueType::SBVec4: r.AsSBVec4 = SBVec4(d4); break;
	case ValueType::USVec4: r.AsUSVec4 = USVec4(d4); break;
	case ValueType::SVec4: r.AsSVec4 = SVec4(d4); break;
	case ValueType::UVec4: r.AsUVec4 = UVec4(d4); break;
	case ValueType::IVec4: r.AsIVec4 = IVec4(d4); break;

	case ValueType::HVec4: r.AsHVec4 = Vector4<HalfFloat>(d4); break;
	case ValueType::Vec4: r.AsVec4 = Vec4(d4); break;
	case ValueType::DVec4: r.AsDVec4 = d4; break;

	default:;
	}
}

}}
