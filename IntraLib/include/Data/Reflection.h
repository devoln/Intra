#pragma once

#include "Data/ValueType.h"
#include "Range/ArrayRange.h"
#include "Meta/Type.h"
#include "Meta/Tuple.h"
#include "Meta/Preprocessor.h"
#include "Memory/SmartRef.h"
#include "Range/StringView.h"

INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS

namespace Intra { namespace Data {

INTRA_DEFINE_EXPRESSION_CHECKER(HasReflectionFieldNamesMethod, T::ReflectionFieldNames());

}}


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
	
#define INTRA_IMPLEMENT_REFLECTION_FIELD_NAMES(A, ...) \
	static ArrayRange<const StringView> ReflectionFieldNames()\
	{\
		static const StringView fieldNames[] = \
		{\
			INTRA_MACRO2_FOR_EACH((,), INTRA_REFLECTION_FIELD_NAME, A, __VA_ARGS__)\
		};\
		return Intra::Range::AsRange(fieldNames);\
	}

//! Добавить метаинформацию к структуре. Первым указывается имя структуры\класса, далее перечисляются поля.
//! Требуется, чтобы были указаны все поля в порядке их объявления.
#define INTRA_ADD_REFLECTION(A, ...) \
	INTRA_IMPLEMENT_REFLECTION_FIELD_NAMES(A, __VA_ARGS__) \
    INTRA_IMPLEMENT_FOR_EACH_FIELD(__VA_ARGS__)\
    INTRA_IMPLEMENT_VISIT_FIELD_BY_ID(__VA_ARGS__)

INTRA_WARNING_POP
