#pragma once

#include "Platform/CppWarnings.h"
#include "Platform/CppFeatures.h"
#include "Meta/Type.h"
#include "Range/AsRange.h"
#include "Containers/Operations.h"

INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS
INTRA_WARNING_DISABLE_SIGN_CONVERSION
INTRA_WARNING_DISABLE_COPY_MOVE_CONSTRUCT_IMPLICITLY_DELETED
INTRA_WARNING_DISABLE_DEFAULT_CONSTRUCTOR_IMPLICITLY_DELETED

namespace Intra { namespace Range {

template<typename C> struct RLastAppender
{
	C& Dst;

	typedef Range::ValueTypeOf<C> value_type;

	void Put(value_type&& v) {Container::AddLast(Dst, Meta::Move(v));}
	void Put(const value_type& v) {Container::AddLast(Dst, v);}
};

template<typename C> struct RFirstAppender
{
	C& Dst;

	typedef Range::ValueTypeOf<C> value_type;

	void Put(value_type&& v) {Container::AddFirst(Dst, Meta::Move(v));}
	void Put(const value_type& v) {Container::AddFirst(Dst, v);}
};

template<typename C> forceinline RLastAppender<C> LastAppender(C& container) {return RLastAppender<C>{container};}
template<typename C> forceinline RFirstAppender<C> FirstAppender(C& container) {return RFirstAppender<C>{container};}

}}

INTRA_WARNING_POP
