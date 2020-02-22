#pragma once

#include "Core/Core.h"

INTRA_BEGIN
struct DateTime
{
	static DateTime Now();
	static uint64 AbsTimeMs();

	uint64 TimeBasedSeed() const;

	ushort Year;
	byte Month, Day, Hour, Minute, Second;

	//! Константа, монотонно зависящая от времени запуска программы.
	//! Его можно использовать как seed в генераторах псевдослучайных чисел.
	static uint64 StartupTimeBasedSeed();
};

template<class R> R& operator<<(R&& stream, const DateTime& dt)
{
	stream << char('0' + dt.Day / 10) << char('0' + dt.Day % 10) << '.' <<
		char('0' + dt.Month / 10) << char('0' + dt.Month % 10) << '.';
	if(dt.Year > 10000) stream << char('0' + dt.Year / 10000);
	if(dt.Year > 1000) stream << char('0' + dt.Year / 1000 % 10);
	if(dt.Year > 100) stream << char('0' + dt.Year / 100 % 10);
	if(dt.Year > 10) stream << char('0' + dt.Year / 10 % 10);
	stream << char('0' + dt.Year % 10);
	stream << ' ' <<
		char('0' + dt.Hour / 10) << char('0' + dt.Hour % 10) << ':' <<
		char('0' + dt.Minute / 10) << char('0' + dt.Minute % 10) << ':' <<
		char('0' + dt.Second / 10) << char('0' + dt.Second % 10);
	return stream;
}
INTRA_END
