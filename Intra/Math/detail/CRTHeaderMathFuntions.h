#pragma once

INTRA_NODISCARD INTRA_MATH_CONSTEXPR forceinline float Floor(float v) {return ::floorf(v);}
INTRA_NODISCARD INTRA_MATH_CONSTEXPR forceinline double Floor(double v) {return ::floor(v);}

INTRA_NODISCARD INTRA_MATH_CONSTEXPR forceinline float Exp(float v) {return ::expf(v);}
INTRA_NODISCARD INTRA_MATH_CONSTEXPR forceinline double Exp(double v) {return ::exp(v);}

INTRA_NODISCARD INTRA_MATH_CONSTEXPR forceinline float Ceil(float v) {return ::ceilf(v);}
INTRA_NODISCARD INTRA_MATH_CONSTEXPR forceinline double Ceil(double v) {return ::ceil(v);}

INTRA_NODISCARD INTRA_MATH_CONSTEXPR forceinline float Round(float v) {return ::roundf(v);}
INTRA_NODISCARD INTRA_MATH_CONSTEXPR forceinline double Round(double v) {return ::round(v);}

INTRA_NODISCARD INTRA_MATH_CONSTEXPR forceinline float Sin(float v) {return ::sinf(v);}
INTRA_NODISCARD INTRA_MATH_CONSTEXPR forceinline double Sin(double v) {return ::sin(v);}

INTRA_NODISCARD INTRA_MATH_CONSTEXPR forceinline float Cos(float v) {return ::cosf(v);}
INTRA_NODISCARD INTRA_MATH_CONSTEXPR forceinline double Cos(double v) {return ::cos(v);}

INTRA_NODISCARD INTRA_MATH_CONSTEXPR forceinline float Sinh(float v) {return ::sinhf(v);}
INTRA_NODISCARD INTRA_MATH_CONSTEXPR forceinline double Sinh(double v) {return ::sinh(v);}

INTRA_NODISCARD INTRA_MATH_CONSTEXPR forceinline float Cosh(float v) {return ::coshf(v);}
INTRA_NODISCARD INTRA_MATH_CONSTEXPR forceinline double Cosh(double v) {return ::cosh(v);}

INTRA_NODISCARD INTRA_MATH_CONSTEXPR forceinline float Tan(float v) {return ::tanf(v);}
INTRA_NODISCARD INTRA_MATH_CONSTEXPR forceinline double Tan(double v) {return ::tan(v);}

INTRA_NODISCARD INTRA_MATH_CONSTEXPR forceinline float Tanh(float v) {return ::tanhf(v);}
INTRA_NODISCARD INTRA_MATH_CONSTEXPR forceinline double Tanh(double v) {return ::tanh(v);}

INTRA_NODISCARD INTRA_MATH_CONSTEXPR forceinline float Acos(float v) {return ::acosf(v);}
INTRA_NODISCARD INTRA_MATH_CONSTEXPR forceinline double Acos(double v) {return ::acos(v);}

INTRA_NODISCARD INTRA_MATH_CONSTEXPR forceinline float Asin(float v) {return ::asinf(v); }
INTRA_NODISCARD INTRA_MATH_CONSTEXPR forceinline double Asin(double v) {return ::asin(v);}

INTRA_NODISCARD INTRA_MATH_CONSTEXPR forceinline float Atan(float v) {return ::atanf(v);}
INTRA_NODISCARD INTRA_MATH_CONSTEXPR forceinline double Atan(double v) {return ::atan(v);}

//INTRA_MATH_CONSTEXPR forceinline float Atanh(float v) {return ::atanhf(v);}
//INTRA_MATH_CONSTEXPR forceinline double Atanh(double v) {return ::Atanh(v);}

INTRA_NODISCARD INTRA_MATH_CONSTEXPR forceinline float Sqrt(float v) {return ::sqrtf(v);}
INTRA_NODISCARD INTRA_MATH_CONSTEXPR forceinline double Sqrt(double v) {return ::sqrt(v);}

INTRA_NODISCARD INTRA_MATH_CONSTEXPR forceinline float Log(float v) {return ::logf(v);}
INTRA_NODISCARD INTRA_MATH_CONSTEXPR forceinline double Log(double v) {return ::log(v);}

INTRA_NODISCARD INTRA_MATH_CONSTEXPR forceinline float Pow(float v, float power) {return ::powf(v, power);}
INTRA_NODISCARD INTRA_MATH_CONSTEXPR forceinline double Pow(double v, double power) {return ::pow(v, power);}

INTRA_NODISCARD INTRA_MATH_CONSTEXPR forceinline float Mod(float x, float y) {return ::fmodf(x, y);}
INTRA_NODISCARD INTRA_MATH_CONSTEXPR forceinline double Mod(double x, double y) {return ::fmod(x, y);}
