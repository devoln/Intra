#pragma once

#include "Platform/InitializerList.h"
#include "Core/FundamentalTypes.h"
#include "Platform/CppWarnings.h"
#include "Meta/Type.h"
#include "Range/Concepts.h"

namespace Intra { namespace Range {

INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS

template<typename T> struct ArrayRange;
template<typename Char> struct GenericStringView;
typedef GenericStringView<char> StringView;
typedef GenericStringView<wchar> WStringView;
typedef GenericStringView<dchar> DStringView;


template<typename T, size_t N> Meta::EnableIf<
	!Meta::IsCharType<T>::_,
ArrayRange<T>> AsRange(T(&arr)[N]);

template<typename T, size_t N> Meta::EnableIf<
	!Meta::IsCharType<T>::_,
ArrayRange<const T>> AsRange(const T(&arr)[N]);


template<typename T> ArrayRange<const T> AsRange(std::initializer_list<T> arr);

template<typename T, size_t N> Meta::EnableIf<
	!Meta::IsCharType<T>::_,
ArrayRange<T>> AsConstRange(T(&arr)[N]);

template<typename T> ArrayRange<const T> AsConstRange(std::initializer_list<T> arr);


template<typename T, size_t N> forceinline Meta::EnableIf<
	Meta::IsCharType<T>::_,
GenericStringView<T>> AsRange(const T(&stringLiteral)[N]);

template<typename T, size_t N> forceinline Meta::EnableIf<
	Meta::IsCharType<T>::_,
GenericStringView<T>> AsConstRange(const T(&arr)[N]);

template<typename T, size_t N> forceinline Meta::EnableIf<
	!Meta::IsCharType<T>::_,
ArrayRange<const T>> AsConstRange(const T(&arr)[N]);

}

using Range::ArrayRange;
using Range::GenericStringView;
using Range::StringView;
using Range::WStringView;
using Range::DStringView;
using Range::AsRange;

INTRA_WARNING_POP

}
