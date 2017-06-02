#pragma once

#include "Fundamental.h"
#include "Warnings.h"
#include "Features.h"

INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS

namespace Intra { namespace Cpp {

#if defined(_MSC_VER) && !defined(__clang__)
#pragma warning(disable: 4056)
constexpr const float Infinity = 1e300*1e300;
#else
constexpr const float Infinity = __builtin_huge_valf();
#endif

struct TNaN
{
#ifndef __FAST_MATH__
	constexpr forceinline bool operator==(float rhs) const {return rhs != rhs;}
	constexpr forceinline bool operator==(double rhs) const {return rhs != rhs;}
	constexpr forceinline bool operator==(real rhs) const {return rhs != rhs;}
#else
	constexpr forceinline bool operator==(float rhs) const {return __builtin_isnan(rhs)!=0;}
	constexpr forceinline bool operator==(double rhs) const {return __builtin_isnan(float(rhs))!=0;}
	constexpr forceinline bool operator==(real rhs) const {return __builtin_isnan(float(rhs))!=0;}
#endif

	constexpr forceinline bool operator!=(float rhs) const {return !operator==(rhs);}
	constexpr forceinline bool operator!=(double rhs) const {return !operator==(rhs);}
	constexpr forceinline bool operator!=(real rhs) const {return !operator==(double(rhs));}

	forceinline operator float() const {return float(Infinity/Infinity);}
	forceinline operator double() const {return double(Infinity/Infinity);}
	forceinline operator real() const {return real(Infinity/Infinity);}

	constexpr TNaN() {}
};
constexpr const TNaN NaN;

constexpr forceinline bool operator==(float l, TNaN) {return NaN == l;}
constexpr forceinline bool operator!=(float l, TNaN) {return NaN != l;}
constexpr forceinline bool operator==(double l, TNaN) {return NaN == l;}
constexpr forceinline bool operator!=(double l, TNaN) {return NaN != l;}
constexpr forceinline bool operator==(real l, TNaN) {return NaN == l;}
constexpr forceinline bool operator!=(real l, TNaN) {return NaN != l;}

}}

INTRA_WARNING_POP
