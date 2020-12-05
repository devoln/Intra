#pragma once

#include "Intra/Core.h"

INTRA_BEGIN

namespace z_D {
template<typename T> [[nodiscard]] INTRA_MATH_CONSTEXPR T Erf1(T x)
{
	constexpr T P[] = {3.16112374387056560, 113.864154151050156, 377.485237685302021};
	constexpr T Q[] = {23.6012909523441209, 244.024637934444173, 1282.61652607737228};
	const T x2 = x*x;
	T xnum = T(T(0.185777706184603153)*x2);
	T xden = x2;
	for(int i = 0; i < 3; i++)
	{
		xnum = T((xnum + P[i]) * x2);
		xden = T((xden + Q[i]) * x2);
	}
	return x * T(xnum + T(3209.37758913846947)) / T(xden + T(2844.23683343917062));
}

template<typename T> [[nodiscard]] INTRA_MATH_CONSTEXPR T Erfc2(T x)
{
	constexpr T P[] = {0.564188496988670089, 8.88314979438837594, 66.1191906371416295,
		298.635138197400131, 881.952221241769090, 1712.04761263407058, 2051.07837782607147};
	constexpr T Q[] = {15.7449261107098347, 117.693950891312499, 537.181101862009858,
		1621.38957456669019, 3290.79923573345963, 4362.61909014324716, 3439.36767414372164};
	T xnum = T(T(2.15311535474403846e-8) * x);
	T xden = x;
	for(int i = 0; i < 7; i++)
	{
		xnum = T((xnum + P[i])*x);
		xden = T((xden + Q[i])*x);
	}
	const T result = T((xnum + T(1230.33935479799725)) / (xden + T(1230.33935480374942)));
	const T xfloor = Floor(x*16) / 16;
	const T del = (x - xfloor) * (x + xfloor);
	return Exp(-Sqr(xfloor) - del) * result;
}

template<typename T> [[nodiscard]] INTRA_MATH_CONSTEXPR T Erfc3(T x)
{
	constexpr T P[] = {3.05326634961232344e-1, 3.60344899949804439e-1, 1.25781726111229246e-1, 1.60837851487422766e-2};
	constexpr T Q[] = {2.56852019228982242e00, 1.87295284992346047e00, 5.27905102951428412e-1, 6.05183413124413191e-2};
	const T x2r = 1 / Sqr(x);
	T xnum = T(T(0.0163153871373020978) * x2r);
	T xden = x2r;
	for(int i = 0; i < 4; i++)
	{
		xnum = T((xnum + P[i]) * x2r);
		xden = T((xden + Q[i]) * x2r);
	}
	T result = T(x2r * (xnum + T(6.58749161529837803e-4)) / (xden + T(2.33520497626869185e-3)));
	result = T((1 / Constants.SqrtPI - result) / x);
	const T xfloor = Floor(x * 16) / 16;
	const T del = (x - xfloor)*(x + xfloor);
	return Exp(-Sqr(xfloor) - del) * result;
}
}

constexpr auto Erf = [](const auto& x) {
	const auto xAbs = Abs(x);
	const auto approx1Arg = sizeof(x) <= 4? 4: (sizeof(x) <= 8? 6: 10);
	if(xAbs >= approx1Arg) return Sign(x);
	if(xAbs <= TRemoveConstRef<decltype(x)>(0.46875)) return z_D::Erf1(x);
	if(xAbs <= 4) return Sign(x)*(1 - z_D::Erfc2(xAbs));
	return Sign(x)*(1 - z_D::Erfc3(xAbs));
};

constexpr auto Erfc = [](const auto& x) {return 1 - Erf(x);};

/// Linear interpolation
template<typename T, typename U>
[[nodiscard]] constexpr T LinearMix(T x, T y, U factor) {return T(x*(U(1) - factor) + y*factor);}

template<typename T> [[nodiscard]] constexpr T SmoothStep(T edge0, T edge1, T value)
{
    const T t = Clamp((value - edge0) / (edge1 - edge0), T(0), T(1));
    return t*t*(T(3) - t*2);
}

INTRA_END
