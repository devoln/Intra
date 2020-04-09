#pragma once

[[nodiscard]] INTRA_MATH_CONSTEXPR INTRA_FORCEINLINE float Floor(float v) {return ::floorf(v);}
[[nodiscard]] INTRA_MATH_CONSTEXPR INTRA_FORCEINLINE double Floor(double v) {return ::floor(v);}

[[nodiscard]] INTRA_MATH_CONSTEXPR INTRA_FORCEINLINE float Exp(float v) {return ::expf(v);}
[[nodiscard]] INTRA_MATH_CONSTEXPR INTRA_FORCEINLINE double Exp(double v) {return ::exp(v);}

[[nodiscard]] INTRA_MATH_CONSTEXPR INTRA_FORCEINLINE float Ceil(float v) {return ::ceilf(v);}
[[nodiscard]] INTRA_MATH_CONSTEXPR INTRA_FORCEINLINE double Ceil(double v) {return ::ceil(v);}

[[nodiscard]] INTRA_MATH_CONSTEXPR INTRA_FORCEINLINE float Round(float v) {return ::roundf(v);}
[[nodiscard]] INTRA_MATH_CONSTEXPR INTRA_FORCEINLINE double Round(double v) {return ::round(v);}

[[nodiscard]] INTRA_MATH_CONSTEXPR INTRA_FORCEINLINE float Sin(float v) {return ::sinf(v);}
[[nodiscard]] INTRA_MATH_CONSTEXPR INTRA_FORCEINLINE double Sin(double v) {return ::sin(v);}

[[nodiscard]] INTRA_MATH_CONSTEXPR INTRA_FORCEINLINE float Cos(float v) {return ::cosf(v);}
[[nodiscard]] INTRA_MATH_CONSTEXPR INTRA_FORCEINLINE double Cos(double v) {return ::cos(v);}

[[nodiscard]] INTRA_MATH_CONSTEXPR INTRA_FORCEINLINE float Sinh(float v) {return ::sinhf(v);}
[[nodiscard]] INTRA_MATH_CONSTEXPR INTRA_FORCEINLINE double Sinh(double v) {return ::sinh(v);}

[[nodiscard]] INTRA_MATH_CONSTEXPR INTRA_FORCEINLINE float Cosh(float v) {return ::coshf(v);}
[[nodiscard]] INTRA_MATH_CONSTEXPR INTRA_FORCEINLINE double Cosh(double v) {return ::cosh(v);}

[[nodiscard]] INTRA_MATH_CONSTEXPR INTRA_FORCEINLINE float Tan(float v) {return ::tanf(v);}
[[nodiscard]] INTRA_MATH_CONSTEXPR INTRA_FORCEINLINE double Tan(double v) {return ::tan(v);}

[[nodiscard]] INTRA_MATH_CONSTEXPR INTRA_FORCEINLINE float Tanh(float v) {return ::tanhf(v);}
[[nodiscard]] INTRA_MATH_CONSTEXPR INTRA_FORCEINLINE double Tanh(double v) {return ::tanh(v);}

[[nodiscard]] INTRA_MATH_CONSTEXPR INTRA_FORCEINLINE float Acos(float v) {return ::acosf(v);}
[[nodiscard]] INTRA_MATH_CONSTEXPR INTRA_FORCEINLINE double Acos(double v) {return ::acos(v);}

[[nodiscard]] INTRA_MATH_CONSTEXPR INTRA_FORCEINLINE float Asin(float v) {return ::asinf(v); }
[[nodiscard]] INTRA_MATH_CONSTEXPR INTRA_FORCEINLINE double Asin(double v) {return ::asin(v);}

[[nodiscard]] INTRA_MATH_CONSTEXPR INTRA_FORCEINLINE float Atan(float v) {return ::atanf(v);}
[[nodiscard]] INTRA_MATH_CONSTEXPR INTRA_FORCEINLINE double Atan(double v) {return ::atan(v);}

//INTRA_MATH_CONSTEXPR INTRA_FORCEINLINE float Atanh(float v) {return ::atanhf(v);}
//INTRA_MATH_CONSTEXPR INTRA_FORCEINLINE double Atanh(double v) {return ::Atanh(v);}

[[nodiscard]] INTRA_MATH_CONSTEXPR INTRA_FORCEINLINE float Sqrt(float v) {return ::sqrtf(v);}
[[nodiscard]] INTRA_MATH_CONSTEXPR INTRA_FORCEINLINE double Sqrt(double v) {return ::sqrt(v);}

[[nodiscard]] INTRA_MATH_CONSTEXPR INTRA_FORCEINLINE float Log(float v) {return ::logf(v);}
[[nodiscard]] INTRA_MATH_CONSTEXPR INTRA_FORCEINLINE double Log(double v) {return ::log(v);}

[[nodiscard]] INTRA_MATH_CONSTEXPR INTRA_FORCEINLINE float Pow(float v, float power) {return ::powf(v, power);}
[[nodiscard]] INTRA_MATH_CONSTEXPR INTRA_FORCEINLINE double Pow(double v, double power) {return ::pow(v, power);}

[[nodiscard]] INTRA_MATH_CONSTEXPR INTRA_FORCEINLINE float Mod(float x, float y) {return ::fmodf(x, y);}
[[nodiscard]] INTRA_MATH_CONSTEXPR INTRA_FORCEINLINE double Mod(double x, double y) {return ::fmod(x, y);}
