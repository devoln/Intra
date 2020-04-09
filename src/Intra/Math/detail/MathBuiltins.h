#pragma once

[[nodiscard]] INTRA_MATH_CONSTEXPR INTRA_FORCEINLINE float Floor(float x) {return __builtin_floorf(x);}
[[nodiscard]] INTRA_MATH_CONSTEXPR INTRA_FORCEINLINE double Floor(double x) {return __builtin_floor(x);}
[[nodiscard]] INTRA_MATH_CONSTEXPR INTRA_FORCEINLINE long double Floor(long double x) {return __builtin_floorl(x);}
[[nodiscard]] INTRA_MATH_CONSTEXPR INTRA_FORCEINLINE float Ceil(float x) {return __builtin_ceilf(x);}
[[nodiscard]] INTRA_MATH_CONSTEXPR INTRA_FORCEINLINE double Ceil(double x) {return __builtin_ceil(x);}
[[nodiscard]] INTRA_MATH_CONSTEXPR INTRA_FORCEINLINE long double Ceil(long double x) {return __builtin_ceill(x);}
[[nodiscard]] INTRA_MATH_CONSTEXPR INTRA_FORCEINLINE float Round(float x) {return __builtin_roundf(x);}
[[nodiscard]] INTRA_MATH_CONSTEXPR INTRA_FORCEINLINE double Round(double x) {return __builtin_round(x);}
[[nodiscard]] INTRA_MATH_CONSTEXPR INTRA_FORCEINLINE long double Round(long double x) {return __builtin_roundl(x);}

[[nodiscard]] INTRA_MATH_CONSTEXPR INTRA_FORCEINLINE float Sin(float radians) {return __builtin_sinf(radians);}
[[nodiscard]] INTRA_MATH_CONSTEXPR INTRA_FORCEINLINE double Sin(double radians) {return __builtin_sin(radians);}
[[nodiscard]] INTRA_MATH_CONSTEXPR INTRA_FORCEINLINE long double Sin(long double radians) {return __builtin_sinl(radians);}
[[nodiscard]] INTRA_MATH_CONSTEXPR INTRA_FORCEINLINE float Cos(float radians) {return __builtin_cosf(radians);}
[[nodiscard]] INTRA_MATH_CONSTEXPR INTRA_FORCEINLINE double Cos(double radians) {return __builtin_cos(radians);}
[[nodiscard]] INTRA_MATH_CONSTEXPR INTRA_FORCEINLINE long double Cos(long double radians) {return __builtin_cosl(radians);}
[[nodiscard]] INTRA_MATH_CONSTEXPR INTRA_FORCEINLINE float Tan(float radians) {return __builtin_tanf(radians);}
[[nodiscard]] INTRA_MATH_CONSTEXPR INTRA_FORCEINLINE double Tan(double radians) {return __builtin_tan(radians);}
[[nodiscard]] INTRA_MATH_CONSTEXPR INTRA_FORCEINLINE long double Tan(long double radians) {return __builtin_tanl(radians);}

[[nodiscard]] INTRA_MATH_CONSTEXPR INTRA_FORCEINLINE float Sinh(float x) {return __builtin_sinhf(x);}
[[nodiscard]] INTRA_MATH_CONSTEXPR INTRA_FORCEINLINE double Sinh(double x) {return __builtin_sinh(x);}
[[nodiscard]] INTRA_MATH_CONSTEXPR INTRA_FORCEINLINE long double Sinh(long double x) {return __builtin_sinhl(x);}
[[nodiscard]] INTRA_MATH_CONSTEXPR INTRA_FORCEINLINE float Cosh(float x) {return __builtin_coshf(x);}
[[nodiscard]] INTRA_MATH_CONSTEXPR INTRA_FORCEINLINE double Cosh(double x) {return __builtin_cosh(x);}
[[nodiscard]] INTRA_MATH_CONSTEXPR INTRA_FORCEINLINE long double Cosh(long double x) {return __builtin_coshl(x);}
[[nodiscard]] INTRA_MATH_CONSTEXPR INTRA_FORCEINLINE float Tanh(float x) {return __builtin_tanhf(x);}
[[nodiscard]] INTRA_MATH_CONSTEXPR INTRA_FORCEINLINE double Tanh(double x) {return __builtin_tanh(x);}
[[nodiscard]] INTRA_MATH_CONSTEXPR INTRA_FORCEINLINE long double Tanh(long double x) {return __builtin_tanhl(x);}

[[nodiscard]] INTRA_MATH_CONSTEXPR INTRA_FORCEINLINE float Asin(float x) {return __builtin_asinf(x);}
[[nodiscard]] INTRA_MATH_CONSTEXPR INTRA_FORCEINLINE double Asin(double x) {return __builtin_asin(x);}
[[nodiscard]] INTRA_MATH_CONSTEXPR INTRA_FORCEINLINE long double Asin(long double x) {return __builtin_asinl(x);}
[[nodiscard]] INTRA_MATH_CONSTEXPR INTRA_FORCEINLINE float Acos(float x) {return __builtin_acosf(x);}
[[nodiscard]] INTRA_MATH_CONSTEXPR INTRA_FORCEINLINE double Acos(double x) {return __builtin_acos(x);}
[[nodiscard]] INTRA_MATH_CONSTEXPR INTRA_FORCEINLINE long double Acos(long double x) {return __builtin_acosl(x);}

[[nodiscard]] INTRA_MATH_CONSTEXPR INTRA_FORCEINLINE float Atan(float x) {return __builtin_atanf(x);}
[[nodiscard]] INTRA_MATH_CONSTEXPR INTRA_FORCEINLINE double Atan(double x) {return __builtin_atan(x);}
[[nodiscard]] INTRA_MATH_CONSTEXPR INTRA_FORCEINLINE long double Atan(long double x) {return __builtin_atanl(x);}

[[nodiscard]] INTRA_MATH_CONSTEXPR INTRA_FORCEINLINE float Atanh(float x) {return __builtin_atanhf(x);}
[[nodiscard]] INTRA_MATH_CONSTEXPR INTRA_FORCEINLINE double Atanh(double x) {return __builtin_atanh(x);}
[[nodiscard]] INTRA_MATH_CONSTEXPR INTRA_FORCEINLINE long double Atanh(long double x) {return __builtin_atanhl(x);}

[[nodiscard]] INTRA_MATH_CONSTEXPR INTRA_FORCEINLINE float Sqrt(float x) {return __builtin_sqrtf(x);}
[[nodiscard]] INTRA_MATH_CONSTEXPR INTRA_FORCEINLINE double Sqrt(double x) {return __builtin_sqrt(x);}
[[nodiscard]] INTRA_MATH_CONSTEXPR INTRA_FORCEINLINE long double Sqrt(long double x) {return __builtin_sqrtl(x);}

[[nodiscard]] INTRA_MATH_CONSTEXPR INTRA_FORCEINLINE float Log(float x) {return __builtin_logf(x);}
[[nodiscard]] INTRA_MATH_CONSTEXPR INTRA_FORCEINLINE double Log(double x) {return __builtin_log(x);}
[[nodiscard]] INTRA_MATH_CONSTEXPR INTRA_FORCEINLINE long double Log(long double x) {return __builtin_logl(x);}

[[nodiscard]] INTRA_MATH_CONSTEXPR INTRA_FORCEINLINE float Mod(float x, float y) {return __builtin_fmodf(x, y);}
[[nodiscard]] INTRA_MATH_CONSTEXPR INTRA_FORCEINLINE double Mod(double x, double y) {return __builtin_fmod(x, y);}
[[nodiscard]] INTRA_MATH_CONSTEXPR INTRA_FORCEINLINE long double Mod(long double x, long double y) {return __builtin_fmodl(x, y);}

[[nodiscard]] INTRA_MATH_CONSTEXPR INTRA_FORCEINLINE float Pow(float x, float power) {return __builtin_powf(x, power);}
[[nodiscard]] INTRA_MATH_CONSTEXPR INTRA_FORCEINLINE double Pow(double x, double power) {return __builtin_pow(x, power);}
[[nodiscard]] INTRA_MATH_CONSTEXPR INTRA_FORCEINLINE long double Pow(long double x, long double power) {return __builtin_powl(x, power);}
[[nodiscard]] INTRA_MATH_CONSTEXPR INTRA_FORCEINLINE float Pow(float x, int power) {return __builtin_powif(x, power);}
[[nodiscard]] INTRA_MATH_CONSTEXPR INTRA_FORCEINLINE double Pow(double x, int power) {return __builtin_powi(x, power);}
[[nodiscard]] INTRA_MATH_CONSTEXPR INTRA_FORCEINLINE long double Pow(long double x, int power) {return __builtin_powil(x, power);}

[[nodiscard]] INTRA_MATH_CONSTEXPR INTRA_FORCEINLINE float Exp(float x) {return __builtin_expf(x);}
[[nodiscard]] INTRA_MATH_CONSTEXPR INTRA_FORCEINLINE double Exp(double x) {return __builtin_exp(x);}
[[nodiscard]] INTRA_MATH_CONSTEXPR INTRA_FORCEINLINE long double Exp(long double x) {return __builtin_expl(x);}
