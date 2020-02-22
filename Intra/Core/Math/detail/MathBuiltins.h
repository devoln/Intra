#pragma once

INTRA_NODISCARD INTRA_MATH_CONSTEXPR forceinline float Floor(float x) {return __builtin_floorf(x);}
INTRA_NODISCARD INTRA_MATH_CONSTEXPR forceinline double Floor(double x) {return __builtin_floor(x);}
INTRA_NODISCARD INTRA_MATH_CONSTEXPR forceinline long double Floor(long double x) {return __builtin_floorl(x);}
INTRA_NODISCARD INTRA_MATH_CONSTEXPR forceinline float Ceil(float x) {return __builtin_ceilf(x);}
INTRA_NODISCARD INTRA_MATH_CONSTEXPR forceinline double Ceil(double x) {return __builtin_ceil(x);}
INTRA_NODISCARD INTRA_MATH_CONSTEXPR forceinline long double Ceil(long double x) {return __builtin_ceill(x);}
INTRA_NODISCARD INTRA_MATH_CONSTEXPR forceinline float Round(float x) {return __builtin_roundf(x);}
INTRA_NODISCARD INTRA_MATH_CONSTEXPR forceinline double Round(double x) {return __builtin_round(x);}
INTRA_NODISCARD INTRA_MATH_CONSTEXPR forceinline long double Round(long double x) {return __builtin_roundl(x);}

INTRA_NODISCARD INTRA_MATH_CONSTEXPR forceinline float Sin(float radians) {return __builtin_sinf(radians);}
INTRA_NODISCARD INTRA_MATH_CONSTEXPR forceinline double Sin(double radians) {return __builtin_sin(radians);}
INTRA_NODISCARD INTRA_MATH_CONSTEXPR forceinline long double Sin(long double radians) {return __builtin_sinl(radians);}
INTRA_NODISCARD INTRA_MATH_CONSTEXPR forceinline float Cos(float radians) {return __builtin_cosf(radians);}
INTRA_NODISCARD INTRA_MATH_CONSTEXPR forceinline double Cos(double radians) {return __builtin_cos(radians);}
INTRA_NODISCARD INTRA_MATH_CONSTEXPR forceinline long double Cos(long double radians) {return __builtin_cosl(radians);}
INTRA_NODISCARD INTRA_MATH_CONSTEXPR forceinline float Tan(float radians) {return __builtin_tanf(radians);}
INTRA_NODISCARD INTRA_MATH_CONSTEXPR forceinline double Tan(double radians) {return __builtin_tan(radians);}
INTRA_NODISCARD INTRA_MATH_CONSTEXPR forceinline long double Tan(long double radians) {return __builtin_tanl(radians);}

INTRA_NODISCARD INTRA_MATH_CONSTEXPR forceinline float Sinh(float x) {return __builtin_sinhf(x);}
INTRA_NODISCARD INTRA_MATH_CONSTEXPR forceinline double Sinh(double x) {return __builtin_sinh(x);}
INTRA_NODISCARD INTRA_MATH_CONSTEXPR forceinline long double Sinh(long double x) {return __builtin_sinhl(x);}
INTRA_NODISCARD INTRA_MATH_CONSTEXPR forceinline float Cosh(float x) {return __builtin_coshf(x);}
INTRA_NODISCARD INTRA_MATH_CONSTEXPR forceinline double Cosh(double x) {return __builtin_cosh(x);}
INTRA_NODISCARD INTRA_MATH_CONSTEXPR forceinline long double Cosh(long double x) {return __builtin_coshl(x);}
INTRA_NODISCARD INTRA_MATH_CONSTEXPR forceinline float Tanh(float x) {return __builtin_tanhf(x);}
INTRA_NODISCARD INTRA_MATH_CONSTEXPR forceinline double Tanh(double x) {return __builtin_tanh(x);}
INTRA_NODISCARD INTRA_MATH_CONSTEXPR forceinline long double Tanh(long double x) {return __builtin_tanhl(x);}

INTRA_NODISCARD INTRA_MATH_CONSTEXPR forceinline float Asin(float x) {return __builtin_asinf(x);}
INTRA_NODISCARD INTRA_MATH_CONSTEXPR forceinline double Asin(double x) {return __builtin_asin(x);}
INTRA_NODISCARD INTRA_MATH_CONSTEXPR forceinline long double Asin(long double x) {return __builtin_asinl(x);}
INTRA_NODISCARD INTRA_MATH_CONSTEXPR forceinline float Acos(float x) {return __builtin_acosf(x);}
INTRA_NODISCARD INTRA_MATH_CONSTEXPR forceinline double Acos(double x) {return __builtin_acos(x);}
INTRA_NODISCARD INTRA_MATH_CONSTEXPR forceinline long double Acos(long double x) {return __builtin_acosl(x);}

INTRA_NODISCARD INTRA_MATH_CONSTEXPR forceinline float Atan(float x) {return __builtin_atanf(x);}
INTRA_NODISCARD INTRA_MATH_CONSTEXPR forceinline double Atan(double x) {return __builtin_atan(x);}
INTRA_NODISCARD INTRA_MATH_CONSTEXPR forceinline long double Atan(long double x) {return __builtin_atanl(x);}

INTRA_NODISCARD INTRA_MATH_CONSTEXPR forceinline float Atanh(float x) {return __builtin_atanhf(x);}
INTRA_NODISCARD INTRA_MATH_CONSTEXPR forceinline double Atanh(double x) {return __builtin_atanh(x);}
INTRA_NODISCARD INTRA_MATH_CONSTEXPR forceinline long double Atanh(long double x) {return __builtin_atanhl(x);}

INTRA_NODISCARD INTRA_MATH_CONSTEXPR forceinline float Sqrt(float x) {return __builtin_sqrtf(x);}
INTRA_NODISCARD INTRA_MATH_CONSTEXPR forceinline double Sqrt(double x) {return __builtin_sqrt(x);}
INTRA_NODISCARD INTRA_MATH_CONSTEXPR forceinline long double Sqrt(long double x) {return __builtin_sqrtl(x);}

INTRA_NODISCARD INTRA_MATH_CONSTEXPR forceinline float Log(float x) {return __builtin_logf(x);}
INTRA_NODISCARD INTRA_MATH_CONSTEXPR forceinline double Log(double x) {return __builtin_log(x);}
INTRA_NODISCARD INTRA_MATH_CONSTEXPR forceinline long double Log(long double x) {return __builtin_logl(x);}

INTRA_NODISCARD INTRA_MATH_CONSTEXPR forceinline float Mod(float x, float y) {return __builtin_fmodf(x, y);}
INTRA_NODISCARD INTRA_MATH_CONSTEXPR forceinline double Mod(double x, double y) {return __builtin_fmod(x, y);}
INTRA_NODISCARD INTRA_MATH_CONSTEXPR forceinline long double Mod(long double x, long double y) {return __builtin_fmodl(x, y);}

INTRA_NODISCARD INTRA_MATH_CONSTEXPR forceinline float Pow(float x, float power) {return __builtin_powf(x, power);}
INTRA_NODISCARD INTRA_MATH_CONSTEXPR forceinline double Pow(double x, double power) {return __builtin_pow(x, power);}
INTRA_NODISCARD INTRA_MATH_CONSTEXPR forceinline long double Pow(long double x, long double power) {return __builtin_powl(x, power);}
INTRA_NODISCARD INTRA_MATH_CONSTEXPR forceinline float Pow(float x, int power) {return __builtin_powif(x, power);}
INTRA_NODISCARD INTRA_MATH_CONSTEXPR forceinline double Pow(double x, int power) {return __builtin_powi(x, power);}
INTRA_NODISCARD INTRA_MATH_CONSTEXPR forceinline long double Pow(long double x, int power) {return __builtin_powil(x, power);}

INTRA_NODISCARD INTRA_MATH_CONSTEXPR forceinline float Exp(float x) {return __builtin_expf(x);}
INTRA_NODISCARD INTRA_MATH_CONSTEXPR forceinline double Exp(double x) {return __builtin_exp(x);}
INTRA_NODISCARD INTRA_MATH_CONSTEXPR forceinline long double Exp(long double x) {return __builtin_expl(x);}
