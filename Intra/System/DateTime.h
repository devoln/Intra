#pragma once

#include "Cpp/Warnings.h"
#include "Cpp/Fundamental.h"
#include "Cpp/PlatformDetect.h"

INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS

namespace Intra { namespace System {

struct DateTime
{
	static DateTime Now();

	ulong64 TimeBasedSeed() const;

	ushort Year;
	byte Month, Day, Hour, Minute, Second;

	//! Константа, монотонно зависящая от времени запуска программы.
	//! Его можно использовать как seed в генераторах псевдослучайных чисел.
	static ulong64 StartupTimeBasedSeed();
};

template<class R> R& operator<<(R&& stream, const DateTime& dt)
{
	stream << '0' + dt.Day / 10 << '0' + dt.Day % 10 << '.' <<
		'0' + dt.Month / 10 << '0' + dt.Month % 10 << '.';
	if(dt.Year > 10000) stream << '0' + dt.Year / 10000;
	if(dt.Year > 1000) stream << '0' + dt.Year / 1000 % 10;
	if(dt.Year > 100) stream << '0' + dt.Year / 100 % 10;
	if(dt.Year > 10) stream << '0' + dt.Year / 10 % 10;
	stream << ' ' << '0' + dt.Hour / 10 << '0' + dt.Hour % 10 << ':' <<
		'0' + dt.Minute / 10 << '0' + dt.Minute % 10 << ':' <<
		'0' + dt.Second / 10 << '0' + dt.Second % 10;
	return stream;
}

}}

INTRA_WARNING_POP
