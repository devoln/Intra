#pragma once

#include "Data/ValueType.h"
#include "Range/ArrayRange.h"
#include "Meta/Type.h"
#include "Meta/TypeList.h"
#include "Meta/Tuple.h"
#include "Meta/Preprocessor.h"
#include "Memory/SmartRef.h"
#include "Range/StringView.h"

namespace Intra { namespace Data {

struct StructReflection;
struct StructFieldDynamicSerializer;

INTRA_DEFINE_EXPRESSION_CHECKER(HasReflection, &T::Reflection());

struct StructField
{
private:
	friend struct StructFieldDynamicSerializer;

	template<typename F> static constexpr Meta::EnableIf<HasReflection<F>::_,
		const StructReflection*> get_reflection() {return &F::Reflection();}

	template<typename F> static constexpr Meta::EnableIf<!HasReflection<F>::_,
		const StructReflection*> get_reflection() {return null;}
public:
	template<typename T, typename F> constexpr StructField(F T::*field):
		Offset(ushort(Meta::MemberOffset(field))),
		Type(ValueType::Of<F>()), Size(ushort(sizeof(F))), SubstructReflection(get_reflection<F>()) {}

	//! Смещение поля структуры в байтах относительно её начала
	ushort Offset;

	//! Если это один из типов перечисления ValueType или их массив, то принимает соответствующее значение.
	//! Если другой POD-тип - Struct.
	//! Для всех остальных типов - End.
	ValueType Type;

	//! Размер поля в байтах (sizeof(FieldType)).
	//! Совпадает с Type.Size()*размер_массива (Type.Size(), если не массив),
	//! если тип относится к перечислению ValueType
	ushort Size;

	//! Для типов с рефлексией указатель на неё.
	//! Для встроенных типов и других типов, не имеющих рефлексии, null.
	const StructReflection* SubstructReflection;
};


struct StructReflection
{
	StructReflection(ArrayRange<const StructField> fields, ArrayRange<const StringView> fieldNames,
		ArrayRange<const StructFieldDynamicSerializer>/* fieldSerializers*/):
		Fields(fields), FieldNames(fieldNames)/*, FieldSerializers(fieldSerializers)*/ {}

	ArrayRange<const StructField> Fields;
	ArrayRange<const StringView> FieldNames;
	//ArrayRange<const StructFieldDynamicSerializer> FieldSerializers;
};

}

/*#include "DynamicSerializer.h"

namespace Data {

struct StructFieldDynamicSerializer
{
	template<typename T, typename F> StructFieldDynamicSerializer(F T::*):
		serializer(((!Meta::TypeListContains<F, ValueType::ValueTypeList>::_ || ValueType::Of<F>()==ValueType::StructureInstance) &&
			StructField::get_reflection<F>()==null)? new DynamicSerializer<F>: null) {}
	const IDynamicSerializer* operator->() const {return serializer.ptr;}
	bool operator==(null_t) const {return serializer==null;}
	bool operator!=(null_t) const {return !operator==(null);}
private:

	//! Сериализатор, который задействуется только при выполнении одновременно всех следующих условий для соответствующего StructField:
	//! 1) Type==End или Type==Struct
	//! 2) SubstructReflection==null
	//! 3) Сериализация не бинарная или тип не POD
	UniqueRef<IDynamicSerializer> serializer;
};

}*/

}


#define INTRA_REFLECTION_FIELD(class, field) {&class::field}
#define INTRA_REFLECTION_TUPLE_FIELD_POINTER(class, field) {&class::field}
#define INTRA_REFLECTION_VISIT(unused, field) visitor(field)
#define INTRA_REFLECTION_VISIT_INDEX(index, field) case index: visitor(field); break
#define INTRA_REFLECTION_TUPLE_FIELD(class, expr) Meta::GetMemberFieldType<decltype(&class::expr)>
#define INTRA_REFLECTION_TUPLE_FIELD_POINTER_TYPE(class, expr) decltype(&class::expr)
#define INTRA_REFLECTION_FIELD_NAME(class, field) #field
//#define INTRA_REFLECTION_TUPLE_FIELD_TEST(class, field) static_assert(offsetof(class, field)==TupleOf::OffsetOf<>);

#define INTRA_IMPLEMENT_FOR_EACH_FIELD(...) \
	template<typename V> void ForEachField(V&& visitor) \
	{INTRA_MACRO2_FOR_EACH((,), INTRA_REFLECTION_VISIT, , __VA_ARGS__);}\
    template<typename V> void ForEachField(V&& visitor) const \
	{INTRA_MACRO2_FOR_EACH((,), INTRA_REFLECTION_VISIT, , __VA_ARGS__);}

#define INTRA_IMPLEMENT_VISIT_FIELD_BY_ID(...) \
	template<typename V> void VisitFieldById(size_t index, V& visitor) {\
        switch(index) {\
            INTRA_MACRO2_FOR_EACH_INDEX((;), INTRA_REFLECTION_VISIT_INDEX, __VA_ARGS__);\
			default: INTRA_ASSERT(!"Invalid id for VisitFieldById.");\
        }\
    }\
    template<typename V> void VisitFieldById(size_t index, V& visitor) const {\
        switch(index) {\
            INTRA_MACRO2_FOR_EACH_INDEX((;), INTRA_REFLECTION_VISIT_INDEX, __VA_ARGS__);\
			default: INTRA_ASSERT(!"Invalid id for VisitFieldById.");\
        }\
    }
	

//! Добавить метаинформацию к структуре. Первым указывается имя типа, далее перечисляются поля.
//! Требуется, чтобы были указаны все поля в порядке их объявления.
#define INTRA_ADD_REFLECTION(A, ...) \
	static ArrayRange<const Data::StructField> ReflectionFields()\
	{\
		static const Data::StructField fields[] = \
		{\
			INTRA_MACRO2_FOR_EACH((,), INTRA_REFLECTION_FIELD, A, __VA_ARGS__)\
		};\
		return Range::AsRange(fields);\
	}\
	static ArrayRange<const StringView> ReflectionFieldNames()\
	{\
		static const StringView fieldNames[] = \
		{\
			INTRA_MACRO2_FOR_EACH((,), INTRA_REFLECTION_FIELD_NAME, A, __VA_ARGS__)\
		};\
		return Range::AsRange(fieldNames);\
	}\
	static const Data::StructReflection& Reflection()\
	{\
		/*static const Data::StructFieldDynamicSerializer fieldSerializers[] = {INTRA_MACRO2_FOR_EACH((,), REFLECTION_FIELD, A, __VA_ARGS__)};*/\
		static const Data::StructReflection result(ReflectionFields(), ReflectionFieldNames(), /*fieldSerializers*/null);\
		return result;\
	}\
    INTRA_IMPLEMENT_FOR_EACH_FIELD(__VA_ARGS__)\
    INTRA_IMPLEMENT_VISIT_FIELD_BY_ID(__VA_ARGS__)

