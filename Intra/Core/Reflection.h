#pragma once

#include "Core/Type.h"
#include "Core/Tuple.h"
#include "Core/Preprocessor.h"
#include "Core/Range/Span.h"

INTRA_BEGIN
INTRA_DEFINE_CONCEPT_REQUIRES(CHasReflectionFieldNamesMethod, TRemoveReference<T>::ReflectionFieldNames());

#define INTRA_REFLECTION_FIELD(class, field) {&class::field}
#define INTRA_REFLECTION_FIELD_POINTER(class, field) {&class::field}
#define INTRA_REFLECTION_VISIT(unused, field) visitor(field)
#define INTRA_REFLECTION_VISIT_INDEX(index, field) case index: visitor(field); break
#define INTRA_REFLECTION_FIELD_TYPE(class, expr) ::Intra::TMemberFieldType<decltype(&class::expr)>
#define INTRA_REFLECTION_TUPLE_FIELD_POINTER_TYPE(class, expr) decltype(&class::expr)
#define INTRA_REFLECTION_FIELD_NAME(class, field) #field
//#define INTRA_REFLECTION_TUPLE_FIELD_TEST(class, field) static_assert(offsetof(class, field) == TupleOf::OffsetOf<>);

#define INTRA_IMPLEMENT_FOR_EACH_FIELD(...) \
	template<typename V> void ForEachField(V&& visitor) \
	{INTRA_MACRO2_FOR_EACH((,), INTRA_REFLECTION_VISIT, , __VA_ARGS__);}\
    template<typename V> void ForEachField(V&& visitor) const \
	{INTRA_MACRO2_FOR_EACH((,), INTRA_REFLECTION_VISIT, , __VA_ARGS__);}

#define INTRA_IMPLEMENT_VISIT_FIELD_BY_ID(...) \
	template<typename V> void VisitFieldById(size_t index, V& visitor) {\
        switch(index) {\
            INTRA_MACRO2_FOR_EACH_INDEX((;), INTRA_REFLECTION_VISIT_INDEX, __VA_ARGS__);\
			default: INTRA_DEBUG_FATAL_ERROR("Invalid id for VisitFieldById.");\
        }\
    }\
    template<typename V> void VisitFieldById(size_t index, V& visitor) const {\
        switch(index) {\
            INTRA_MACRO2_FOR_EACH_INDEX((;), INTRA_REFLECTION_VISIT_INDEX, __VA_ARGS__);\
			default: INTRA_DEBUG_FATAL_ERROR("Invalid id for VisitFieldById.");\
        }\
    }
	
#define INTRA_IMPLEMENT_REFLECTION_FIELD_NAMES(A, ...) \
	static ::Intra::CSpan<const char*> ReflectionFieldNames()\
	{\
		static const char* const fieldNames[] = {\
			INTRA_MACRO2_FOR_EACH((,), INTRA_REFLECTION_FIELD_NAME, A, __VA_ARGS__)\
		};\
		return ::Intra::CSpanOf(fieldNames);\
	}

/** Adds meta information about fields.
  The first argument is the class/struct name, next are the fields in the order of their declaration.
*/
#define INTRA_ADD_FIELD_REFLECTION(A, ...) \
	using Reflection_FieldTypes = ::Intra::TList<INTRA_MACRO2_FOR_EACH((,), INTRA_REFLECTION_FIELD_TYPE, A, __VA_ARGS__)>; \
	INTRA_IMPLEMENT_REFLECTION_FIELD_NAMES(A, __VA_ARGS__) \
    INTRA_IMPLEMENT_FOR_EACH_FIELD(__VA_ARGS__) \
    INTRA_IMPLEMENT_VISIT_FIELD_BY_ID(__VA_ARGS__)

INTRA_END
