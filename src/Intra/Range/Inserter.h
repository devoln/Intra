#pragma once

#include "Intra/Type.h"


INTRA_BEGIN
INTRA_IGNORE_WARNING_SIGN_CONVERSION
INTRA_IGNORE_WARNING_COPY_MOVE_CONSTRUCT_IMPLICITLY_DELETED
INTRA_IGNORE_WARNING_DEFAULT_CONSTRUCTOR_IMPLICITLY_DELETED
template<typename C> struct RLastAppender
{
	C& Dst;

	typedef typename C::value_type value_type;

	constexpr void Put(value_type&& v) {Dst.push_back(Move(v));}
	constexpr void Put(const value_type& v) {Dst.push_back(v);}
};

template<typename C> struct RFirstAppender
{
	C& Dst;

	typedef typename C::value_type value_type;

	constexpr void Put(value_type&& v) {Dst.push_front(Move(v));}
	constexpr void Put(const value_type& v) {Dst.push_front(v);}
};

template<typename C> [[nodiscard]] constexpr RLastAppender<C> LastAppender(C& container) {return RLastAppender<C>{container};}
template<typename C> [[nodiscard]] constexpr RFirstAppender<C> FirstAppender(C& container) {return RFirstAppender<C>{container};}
INTRA_END
