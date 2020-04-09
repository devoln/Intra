#pragma once

#include "Intra/Preprocessor.h"
#include "Intra/Container/Tuple.h"
#include "Intra/Type.h"

INTRA_BEGIN
#define INTRA_DETAIL_REFLECTION_FIELD_NAME(class, field) #field
#define INTRA_DETAIL_REFLECTION_FIELD_NAMES(implName, template, T, ...) \
	namespace z_D {template constexpr const char* implName[] = { \
		INTRA_MACRO2_FOR_EACH((,), INTRA_DETAIL_REFLECTION_FIELD_NAME, T, __VA_ARGS__) \
	};} \
	template constexpr decltype(auto) FieldNamesOf(const T&) noexcept {return static_cast< \
		const char* const(&)[sizeof(implName)/sizeof(implName[0])]>(implName);\
	}

#define INTRA_DETAIL_REFLECTION_FIELD_POINTER(class, field) &class::field
#define INTRA_DETAIL_REFLECTION_FIELD_POINTERS(template, T, ...) \
	template constexpr auto FieldPointersOf(const T&) noexcept {return Tuple{ \
		INTRA_MACRO2_FOR_EACH((,), INTRA_DETAIL_REFLECTION_FIELD_POINTER, T, __VA_ARGS__)\
	}}

/** Add meta information about fields.
  The first argument is the class/struct name, next are the fields in the order of their declaration.
*/
#define INTRA_ADD_FIELD_REFLECTION(T, ...) \
	INTRA_DETAIL_REFLECTION_FIELD_NAMES(INTRA_CONCATENATE_TOKENS(FieldNames_, __COUNTER__),, T, __VA_ARGS__) \
    INTRA_DETAIL_REFLECTION_FIELD_POINTERS(, T, __VA_ARGS__)
INTRA_END
