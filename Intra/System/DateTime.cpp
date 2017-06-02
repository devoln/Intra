#include "DateTime.h"

#include "Cpp/Warnings.h"

INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS

#ifdef _MSC_VER
#define _CRT_SECURE_NO_DEPRECATE
#define _CRT_SECURE_NO_WARNINGS
#endif

#include <time.h>


namespace Intra { namespace System {

DateTime DateTime::Now()
{
	const time_t t = time(0);
	tm* now = localtime(&t);
	return {ushort(now->tm_year+1900), byte(now->tm_mon+1), byte(now->tm_mday),
		byte(now->tm_hour), byte(now->tm_min), byte(now->tm_sec)};
}

ulong64 DateTime::TimeBasedSeed() const
{
	return Second + Minute*60ull + Hour*3600ull +
		Day*86400ull + Month*31ull*86400ull + Year*366ull*86400ull;
}

ulong64 DateTime::StartupTimeBasedSeed()
{
	static const ulong64 result = Now().TimeBasedSeed();
	return result;
}

}}

INTRA_WARNING_POP
