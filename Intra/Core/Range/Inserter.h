#pragma once

#include "Core/Type.h"


INTRA_CORE_RANGE_BEGIN
INTRA_WARNING_DISABLE_SIGN_CONVERSION
INTRA_WARNING_DISABLE_COPY_MOVE_CONSTRUCT_IMPLICITLY_DELETED
INTRA_WARNING_DISABLE_DEFAULT_CONSTRUCTOR_IMPLICITLY_DELETED
template<typename C> struct RLastAppender
{
	C& Dst;

	typedef typename C::value_type value_type;

	INTRA_CONSTEXPR2 void Put(value_type&& v) {Dst.push_back(Move(v));}
	INTRA_CONSTEXPR2 void Put(const value_type& v) {Dst.push_back(v);}
};

template<typename C> struct RFirstAppender
{
	C& Dst;

	typedef typename C::value_type value_type;

	INTRA_CONSTEXPR2 void Put(value_type&& v) {Dst.push_front(Move(v));}
	INTRA_CONSTEXPR2 void Put(const value_type& v) {Dst.push_front(v);}
};

template<typename C> INTRA_NODISCARD constexpr forceinline RLastAppender<C> LastAppender(C& container) {return RLastAppender<C>{container};}
template<typename C> INTRA_NODISCARD constexpr forceinline RFirstAppender<C> FirstAppender(C& container) {return RFirstAppender<C>{container};}
INTRA_CORE_RANGE_END
