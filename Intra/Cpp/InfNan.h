#pragma once

#include "Fundamental.h"
#include "Warnings.h"
#include "Features.h"

INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS

namespace Intra { namespace Cpp {

#if defined(_MSC_VER) && !defined(__clang__)
#pragma warning(disable: 4056)
static const float Infinity = float(1e300*1e300);
#else
constexpr const float Infinity = __builtin_huge_valf();
#endif

struct TNaN
{
#ifndef __FAST_MATH__
	forceinline bool operator==(float rhs) const {return rhs != rhs;}
	forceinline bool operator==(double rhs) const {return rhs != rhs;}
	forceinline bool operator==(real rhs) const {return rhs != rhs;}
#else
	forceinline bool operator==(float rhs) const {return __builtin_isnan(rhs)!=0;}
	forceinline bool operator==(double rhs) const {return __builtin_isnan(float(rhs))!=0;}
	forceinline bool operator==(real rhs) const {return __builtin_isnan(float(rhs))!=0;}
#endif

	forceinline bool operator!=(float rhs) const {return !operator==(rhs);}
	forceinline bool operator!=(double rhs) const {return !operator==(rhs);}
	forceinline bool operator!=(real rhs) const {return !operator==(double(rhs));}

	forceinline operator float() const {return float(Infinity/Infinity);}
	forceinline operator double() const {return double(Infinity/Infinity);}
	forceinline operator real() const {return real(Infinity/Infinity);}

	TNaN() {}
};
static const TNaN NaN;

forceinline bool operator==(float l, TNaN) {return NaN == l;}
forceinline bool operator!=(float l, TNaN) {return NaN != l;}
forceinline bool operator==(double l, TNaN) {return NaN == l;}
forceinline bool operator!=(double l, TNaN) {return NaN != l;}
forceinline bool operator==(real l, TNaN) {return NaN == l;}
forceinline bool operator!=(real l, TNaN) {return NaN != l;}

}}

INTRA_WARNING_POP
