#pragma once

#include "Core/Float.h"
#include "Core/Misc/UintToString.h"

INTRA_CORE_BEGIN
namespace Misc {
template<typename T> INTRA_CONSTEXPR2 index_t DecimalToStringScientific(DecimalFloat<T> x, char decimalSep, char e, char* dst)
{
	index_t len = UintToString(x.Mantissa, dst+1);
	dst[0] = dst[1];
	if(len > 1)
	{
		dst[1] = decimalSep;
		len++;
	}

	int exp = x.Exponent - int(len - 1);
	if(exp == 0) return len;
	dst[len++] = e;
	if(exp < 0)
	{
		exp = -exp;
		dst[len++] = '-';
	}
	len += UintToString(uint32(exp), dst+len);
	return len;
}

template<typename T> INTRA_CONSTEXPR2 index_t DecimalToString(DecimalFloat<T> x, char decimalSep, char e, char* dst)
{
	enum {
		MantissaDigits = int(sizeof(T)*5+1)/2,
		LeadingZerosBeforeSciFormat = 4
	};
	bool fixedReprIsTooLong = x.Exponent > MantissaDigits || x.Exponent < -MantissaDigits-LeadingZerosBeforeSciFormat;
	if(fixedReprIsTooLong) return DecimalToStringScientific(x, decimalSep, e, dst);
	index_t len = UintToString(x.Mantissa, dst+1);
	index_t lenOfFixedFormat = len;
	if(x.Exponent >= 0) lenOfFixedFormat += x.Exponent;
	else
	{
		lenOfFixedFormat += 1;
		if(-x.Exponent >= len) lenOfFixedFormat += 1 - (len + x.Exponent);
	}
	dst[0] = dst[1];
	if(len > 1)
	{
		dst[1] = decimalSep;
		len++;
	}

	int exp = x.Exponent - int(len - 1);
	if(exp == 0) return len;
	dst[len++] = e;
	if(exp < 0)
	{
		exp = -exp;
		dst[len++] = '-';
	}
	len += UintToString(uint32(exp), dst+len);
	return len;
}
}
INTRA_CORE_END
