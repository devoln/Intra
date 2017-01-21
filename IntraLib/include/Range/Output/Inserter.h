#pragma once

#include "Platform/CppWarnings.h"
#include "Platform/CppFeatures.h"
#include "Meta/Type.h"

INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS
INTRA_WARNING_DISABLE_SIGN_CONVERSION

namespace Intra {

namespace Container {
INTRA_DEFINE_EXPRESSION_CHECKER2(HasAddLast, Meta::Val<T1>().AddLast(Meta::Val<T2>()),,);
INTRA_DEFINE_EXPRESSION_CHECKER2(HasAddFirst, Meta::Val<T1>().AddFirst(Meta::Val<T2>()),,);
INTRA_DEFINE_EXPRESSION_CHECKER2(Has_push_back, Meta::Val<T1>().push_back(Meta::Val<T2>()),,);
INTRA_DEFINE_EXPRESSION_CHECKER2(Has_push_front, Meta::Val<T1>().push_front(Meta::Val<T2>()),,);
}

namespace Range {

template<typename C> struct RLastAppender
{
	C& Dst;

	template<typename T> Meta::EnableIf<
		Container::HasAddLast<C, T>::_
	> Put(T&& v) {Dst.AddLast(v);}

	template<typename T> Meta::EnableIf<
		!Container::HasAddLast<C, T>::_ && Container::Has_push_back<C, T>::_
	> Put(T&& v) {Dst.push_back(v);}
};

template<typename C> struct RFirstAppender
{
	C& Dst;

	template<typename T> Meta::EnableIf<
		Container::HasAddFirst<C, T>::_
	> Put(T&& v) {Dst.AddFirst(v);}

	template<typename T> Meta::EnableIf<
		!Container::HasAddFirst<C, T>::_ && Container::Has_push_front<C, T>::_
	> Put(T&& v) {Dst.push_front(v);}
};

}}

INTRA_WARNING_POP
