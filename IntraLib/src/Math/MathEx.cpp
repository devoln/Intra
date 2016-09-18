#include "Math/MathEx.h"
#include <math.h>

#include <float.h>

#ifdef _MSC_VER
#define isnan(x) _isnan(x)
#endif

//#define isnan(x) _isnan(x)
#if defined(_MSC_VER) && _MSC_VER<1800
#define isinf(x) (!_finite(x))
#endif
/*inline float _apowf(float _X, float _Y)
{
	return (float)powf(_X, _Y);
}*/


namespace Intra { namespace Math {


#ifndef INTRA_INLINE_MATH
float Exp(float v) {return ::expf(v);}
double Exp(double v) {return ::exp(v);}

float Floor(float v) {return floorf(v);}
double Floor(double v) {return ::floor(v);}

float Ceil(float v) {return ceilf(v);}
double Ceil(double v) {return ::ceil(v);}

float Round(float v) {return roundf(v);}
double Round(double v) {return ::round(v);}

float Sin(float v) {return sinf(v);}
double Sin(double v) {return ::sin(v);}

float Cos(float v) {return cosf(v);}
double Cos(double v) {return ::cos(v);}

float Sinh(float v) {return sinhf(v);}
double Sinh(double v) {return ::sinh(v);}

float Cosh(float v) {return coshf(v);}
double Cosh(double v) {return ::cosh(v);}

float Tan(float v) {return tanf(v);}
double Tan(double v) {return ::tan(v);}

float Tanh(float v) {return tanhf(v);}
double Tanh(double v) {return ::tanh(v);}

float Acos(float v) {return acosf(v);}
double Acos(double v) {return ::acos(v);}

float Asin(float v) {return asinf(v); }
double Asin(double v) {return ::asin(v);}

float Atan(float v) {return atanf(v);}
double Atan(double v) {return ::atan(v);}

//float Atanh(float v) {return atanhf(v);}
//double Atanh(double v) {return ::Atanh(v);}

float Sqrt(float v) {return sqrtf(v);}
double Sqrt(double v) {return ::sqrt(v);}

float Log(float v) {return logf(v);}
double Log(double v) {return ::log(v);}

float Pow(float v, float power) {return Exp(Log(v)*power);}
double Pow(double v, double power) {return Exp(Log(v)*power);}

float Mod(float x, float y) {return fmodf(x, y);}
double Mod(double x, double y) {return fmod(x, y);}
#endif


bool NaNType::operator==(float rhs) const {return isnan(rhs)!=0;}
bool NaNType::operator==(double rhs) const {return isnan(rhs)!=0;}
bool NaNType::operator==(real rhs) const {return isnan(double(rhs))!=0;}

}}
