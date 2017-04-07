#pragma once

#include "Platform/InitializerList.h"
#include "Platform/FundamentalTypes.h"
#include "Platform/CppWarnings.h"
#include "Meta/Type.h"
#include "Range/Concepts.h"

namespace Intra { namespace Range {

INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS

template<typename T> struct ArrayRange;
template<typename Char> struct GenericStringView;
typedef GenericStringView<const char> StringView;
typedef GenericStringView<const wchar> WStringView;
typedef GenericStringView<const dchar> DStringView;

INTRA_DEFINE_EXPRESSION_CHECKER(HasNextListNodeMethod, ::Intra::Meta::Val<T>().NextListNode());
INTRA_DEFINE_EXPRESSION_CHECKER(HasPrevListNodeMethod, ::Intra::Meta::Val<T>().PrevListNode());
template<typename T> struct FListNode;
template<typename T> struct BListNode;
template<typename T, typename Node = FListNode<T>> struct FListRange;
template<typename T, typename Node = BListNode<T>> struct BListRange;

template<typename I1, typename I2=I1> struct IteratorRange;


template<typename T, size_t N> Meta::EnableIf<
	!Meta::IsCharType<T>::_,
ArrayRange<T>> AsRange(T(&arr)[N]);

template<typename T> ArrayRange<const T> AsRange(InitializerList<T> arr);

template<typename T, size_t N> forceinline Meta::EnableIf<
	Meta::IsCharType<T>::_,
GenericStringView<const T>> AsRange(const T(&stringLiteral)[N]);

template<typename T, size_t N> forceinline Meta::EnableIf<
	Meta::IsCharType<T>::_,
GenericStringView<T>> AsRange(T(&charArr)[N]);

template<typename T> Meta::EnableIf<
	!IsInputRange<T>::_ &&
	HasNextListNodeMethod<T>::_/* &&
	!HasPrevListNodeMethod<T>::_*/,
FListRange<T, T>> AsRange(T& objectWithIntrusiveList);

}

using Range::ArrayRange;
using Range::GenericStringView;
using Range::StringView;
using Range::WStringView;
using Range::DStringView;
using Range::AsRange;

namespace Tags {

enum TKeepTerminator: bool {KeepTerminator = true};

}

INTRA_WARNING_POP

}
