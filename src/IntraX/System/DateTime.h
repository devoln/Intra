#pragma once

#include "IntraX/Core.h"
#include "Intra/Numeric.h"

INTRA_BEGIN
struct SystemTimestamp
{
	static SystemTimestamp Now();

	constexpr int64 NsSince1970() const {return RawValueNs;}
	constexpr int64 SecondsSince1970() const {return (NsSince1970() + 500000000) / 1000000000;}

	int64 RawValueNs = MinValueOf<int64>;
};

struct MonotonicTimestamp
{
	static MonotonicTimestamp Now();
	int64 RawValueNs = MinValueOf<int64>;
};

struct Timestamp
{
	[[nodiscard]] static Timestamp Now() {return {SystemTimestamp::Now(), MonotonicTimestamp::Now()};}

	/// Use this to create a timestamp if you have just obtained the actual time from a server, GPS, or any other time source
	[[nodiscard]] static Timestamp Now(SystemTimestamp systemTime) {return {systemTime, MonotonicTimestamp::Now()};}

	[[nodiscard]] constexpr SystemTimestamp SynchronizedSystemTime(Timestamp timeSource) const
	{
		timeSource.SystemTime.RawValueNs -= timeSource.MonotonicTime.RawValueNs - MonotonicTime.RawValueNs;
		return timeSource.SystemTime;
	}
	[[nodiscard]] SystemTimestamp SynchronizedSystemTime() const {return SynchronizedSystemTime(Now());}

	[[nodiscard]] constexpr int64 NsSince1970() const {return SystemTimestamp(*this).RawValueNs;}
	[[nodiscard]] constexpr int64 SecondsSince1970() const {return (NsSince1970() + 500000000) / 1000000000;}

	SystemTimestamp SystemTime;
	MonotonicTimestamp MonotonicTime;
};

struct TimeDelta
{
	[[nodiscard]] constexpr double Nanoseconds() const {return ValueNs;}
	[[nodiscard]] constexpr double Microseconds() const {return ValueNs / 1000.0;}
	[[nodiscard]] constexpr double Milliseconds() const {return ValueNs / 1000000.0;}
	[[nodiscard]] constexpr double Seconds() const {return ValueNs / 1000000000.0;}
	[[nodiscard]] constexpr double Minutes() const {return ValueNs / 60000000000.0;}
	[[nodiscard]] constexpr double Hours() const {return ValueNs / 3600000000000.0;}
	[[nodiscard]] constexpr double Days() const {return ValueNs / 86400000000000.0;}

	[[nodiscard]] constexpr int64 RoundedNanoseconds() const {return ValueNs;}
	[[nodiscard]] constexpr int64 RoundedMicroseconds() const {return (ValueNs + 500) / 1000;}
	[[nodiscard]] constexpr int64 RoundedMilliseconds() const {return (ValueNs + 500000) / 1000000;}
	[[nodiscard]] constexpr int64 RoundedSeconds() const {return (ValueNs + 500000000) / 1000000000;}
	[[nodiscard]] constexpr int64 RoundedMinutes() const {return (ValueNs + 30000000000) / 60000000000;}
	[[nodiscard]] constexpr int64 RoundedHours() const {return (ValueNs + 1800000000000) / 3600000000000;}
	[[nodiscard]] constexpr int64 RoundedDays() const {return (ValueNs + 43200000000000) / 86400000000000;}

	template<typename T> [[nodiscard]] static constexpr TimeDelta Nanoseconds(T n) const {return {int64(n)};}
	template<typename T> [[nodiscard]] static constexpr TimeDelta Microseconds(T n) const {return {int64(n*1000LL)};}
	template<typename T> [[nodiscard]] static constexpr TimeDelta Milliseconds(T n) const {return {int64(n*1000000LL)};}
	template<typename T> [[nodiscard]] static constexpr TimeDelta Seconds(T n) const {return {int64(n*1000000000LL)};}
	template<typename T> [[nodiscard]] static constexpr TimeDelta Minutes(T n) const {return {int64(n*60000000000LL)};}
	template<typename T> [[nodiscard]] static constexpr TimeDelta Hours(T n) const {return {int64(n*3600000000000LL)};}
	template<typename T> [[nodiscard]] static constexpr TimeDelta Days(T n) const {return {int64(n*86400000000000LL)};}

	int64 ValueNs = 0;
};

[[nodiscard]] constexpr TimeDelta operator""_ns(long double n) {return TimeDelta::Nanoseconds(n);}
[[nodiscard]] constexpr TimeDelta operator""_us(long double n) {return TimeDelta::Microseconds(n);}
[[nodiscard]] constexpr TimeDelta operator""_ms(long double n) {return TimeDelta::Milliseconds(n);}
[[nodiscard]] constexpr TimeDelta operator""_s(long double n) {return TimeDelta::Seconds(n);}
[[nodiscard]] constexpr TimeDelta operator""_min(long double n) {return TimeDelta::Minutes(n);}
[[nodiscard]] constexpr TimeDelta operator""_h(long double n) {return TimeDelta::Hours(n);}
[[nodiscard]] constexpr TimeDelta operator""_d(long double n) {return TimeDelta::Days(n);}

struct DateTime
{
	uint16 Year = 0;
	uint8 Month = 0, Day = 0, Hour = 0, Minute = 0, Second = 0;
	uint8 WeekDay = 0;
	uint16 YearDay = 0;
	int8 LocalTimeOffsetInQuarterHours = 0;

	static DateTime UtcNow()
	{
		return FromTimestamp(SystemTimestamp::Now(), 0);
	}

	static constexpr bool IsLeapYear(int year) const noexcept
	{
		return year % 4 == 0 && year / 100 % 4 == 0;
	}

	constexpr SystemTimestamp ToTimestamp() const
	{
		const auto leapYearsBefore = [](uint32 y) {y += 999999999; return (y / 4) - (y / 100) + (y / 400);};
		const int numLeapYearsSince1970 = leapYearsBefore(Year) - leapYearsBefore(1971);
		const bool isLeapYear = IsLeapYear(Year);
		const int32 daysSinceJan1ThisYear = Day - 1 +
			daysSinceJanNotLeap[Month - 1] + (IsLeapYear(Year) && Month >= 3);
		const int32 numDaysSince1970 = daysSinceJan1ThisYear +
			(Year - 1970) * 365 + numLeapYearsSince1970;
		int64 seconds = Second + Minute * 60 +
			(4*Hour - LocalTimeOffsetInQuarterHours) * 900;
		seconds += numDaysSince1970 * 86400LL;
		return Timestamp{seconds * 1000000000LL};
	}

	static constexpr DateTime FromTimestamp(SystemTimestamp timestamp, int localTimeOffsetInQuarterHours)
	{
		const int64 t = timestamp.SecondsSince1970() + localTimeOffsetInQuarterHours * 900;
		int days = t / 86400;
		int rem = t % 86400;

		DateTime result;
		result.LocalTimeOffsetInQuarterHours = localTimeOffsetInQuarterHours;
		result.Hour = uint8(rem / 3600);
		rem %= 3600;
		result.Minute = uint8(rem / 60);
		result.Second = uint8(rem % 60);
		result.WeekDay = uint8((7 + 4 + days) % 7); // January 1, 1970 was a Thursday

		int y = 1970;
		const auto leapYearsBefore = [](int y) {
			const auto div = [](int a, int b) {return a/b - (a%b < 0);};
			return div(y, 4) - div(y, 100) + div(y, 400);
		};

		while(days < 0 || days >= 365 + IsLeapYear(y))
		{
			// Guess a corrected year, assuming 365 days per year.
			const int yGuessed = y + days / 365 - (days % 365 < 0);

			// Adjust days and y to match the guessed year.
			days -= ((yGuessed - y) * 365
				+ leapYearsBefore(yGuessed - 1)
				- leapYearsBefore(y - 1));
			y = yGuessed;
		}
		result.Year = uint16(y);
		result.YearDay = uint16(days);
		
		const bool isLeapYear = IsLeapYear(Year);
		for(int m = 11;; m--)
		{
			const auto daysSinceJan = daysSinceJanNotLeap[m] + (isLeapYear && m >= 2);
			if(days < daysSinceJan) continue;
			days -= daysSinceJan;
			result.Month = 1 + m;
			result.Day = 1 + days;
			break;
		}
		return result;
	}
private:
	static constexpr uint16 daysSinceJanNotLeap[] = {0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334};
};

template<class R> constexpr R& operator<<(R&& stream, const DateTime& dt)
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
