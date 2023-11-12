#pragma once

#include <Intra/Core.h>
#include <Intra/Numeric/Traits.h>

namespace Intra { INTRA_BEGIN
template<CNumber T> struct GenericTimeDelta
{
	static constexpr auto StepNanoseconds = MaxStepInRange<T> * 1000000000;

	GenericTimeDelta() = default;
	GenericTimeDelta(CNumber auto&& seconds): ValueInSeconds(seconds) {}
	template<CNumber T2> GenericTimeDelta(GenericTimeDelta<T2> t): ValueInSeconds(t.ValueInSeconds) {}

	[[nodiscard]] INTRA_FORCEINLINE constexpr double Nanoseconds() const {return ValueInSeconds * 1000000000.0;}
	[[nodiscard]] INTRA_FORCEINLINE constexpr double Microseconds() const {return ValueInSeconds * 1000000.0;}
	[[nodiscard]] INTRA_FORCEINLINE constexpr double Milliseconds() const {return ValueInSeconds * 1000.0;}
	[[nodiscard]] INTRA_FORCEINLINE constexpr double Seconds() const {return ValueInSeconds;}
	[[nodiscard]] INTRA_FORCEINLINE constexpr double Minutes() const {return ValueInSeconds / 60.0;}
	[[nodiscard]] INTRA_FORCEINLINE constexpr double Hours() const {return ValueInSeconds / 3600.0;}
	[[nodiscard]] INTRA_FORCEINLINE constexpr double Days() const {return ValueInSeconds / 86400.0;}

	[[nodiscard]] INTRA_FORCEINLINE constexpr int64 IntNanoseconds() const {
		if constexpr(StepNanoseconds == 1) return ValueInSeconds.Raw;
		else return int64(ValueInSeconds * 1000000000);
	}
	[[nodiscard]] INTRA_FORCEINLINE constexpr int64 IntMicroseconds() const {return (IntNanoseconds() + 500) / 1000;}
	[[nodiscard]] INTRA_FORCEINLINE constexpr int64 IntMilliseconds() const {return (IntNanoseconds() + 500000) / 1000000;}
	[[nodiscard]] INTRA_FORCEINLINE constexpr int64 IntSeconds() const {return (IntNanoseconds() + 500000000) / 1000000000;}
	[[nodiscard]] INTRA_FORCEINLINE constexpr int64 IntMinutes() const {return (IntNanoseconds() + 30000000000LL) / 60000000000LL;}
	[[nodiscard]] INTRA_FORCEINLINE constexpr int64 IntHours() const {return (IntNanoseconds() + 1800000000000LL) / 3600000000000LL;}
	[[nodiscard]] INTRA_FORCEINLINE constexpr int64 IntDays() const {return (IntNanoseconds() + 43200000000000LL) / 86400000000000LL;}

	[[nodiscard]] static INTRA_FORCEINLINE constexpr GenericTimeDelta Nanoseconds(double n) const {return {T(n * 0.000000001)};}
	[[nodiscard]] static INTRA_FORCEINLINE constexpr GenericTimeDelta Microseconds(double n) const {return {T(n * 0.000001)};}
	[[nodiscard]] static INTRA_FORCEINLINE constexpr GenericTimeDelta Milliseconds(double n) const {return {T(n * 0.001)};}
	[[nodiscard]] static INTRA_FORCEINLINE constexpr GenericTimeDelta Seconds(CNumber auto&& n) const {return {T(n)};}
	[[nodiscard]] static INTRA_FORCEINLINE constexpr GenericTimeDelta Minutes(CNumber auto&& n) const {return {T(n * 60)};}
	[[nodiscard]] static INTRA_FORCEINLINE constexpr GenericTimeDelta Hours(CNumber auto&& n) const {return {T(n * 36000)};}
	[[nodiscard]] static INTRA_FORCEINLINE constexpr GenericTimeDelta Days(CNumber auto&& n) const {return {T(n * 864000)};}

	[[nodiscard]] INTRA_FORCEINLINE constexpr GenericTimeDelta operator+(const GenericTimeDelta& rhs) const {return {ValueInSeconds + rhs.ValueInSeconds};}
	[[nodiscard]] INTRA_FORCEINLINE constexpr GenericTimeDelta operator-(const GenericTimeDelta& rhs) const {return {ValueInSeconds - rhs.ValueInSeconds};}
	[[nodiscard]] INTRA_FORCEINLINE constexpr GenericTimeDelta operator*(CNumber auto rhs) const {return {T(ValueInSeconds * rhs)};}
	[[nodiscard]] INTRA_FORCEINLINE constexpr GenericTimeDelta operator/(CNumber auto rhs) const {return {T(ValueInSeconds / rhs)};}

	template<typename T> [[nodiscard]] INTRA_FORCEINLINE constexpr T To() const
	{
		T res{};
		if constexpr(requires {T().tv_sec;}) res.tv_sec = decltype(res.tv_sec)(IntNanoseconds() / 1000000000);
		if constexpr(requires {T().tv_nsec;}) res.tv_nsec = int32(IntNanoseconds() % 1000000000);
		if constexpr(requires {T().tv_usec;}) res.tv_usec = int32(IntNanoseconds() / 1000 % 1000000);
		return res;
	}

	T ValueInSeconds = 0;
};
using TimeDelta = GenericTimeDelta<Fixed<int64, 1000000000>>;

[[nodiscard]] constexpr TimeDelta operator""_ns(long double n) {return TimeDelta::Nanoseconds(n);}
[[nodiscard]] constexpr TimeDelta operator""_us(long double n) {return TimeDelta::Microseconds(n);}
[[nodiscard]] constexpr TimeDelta operator""_ms(long double n) {return TimeDelta::Milliseconds(n);}
[[nodiscard]] constexpr TimeDelta operator""_s(long double n) {return TimeDelta::Seconds(n);}
[[nodiscard]] constexpr TimeDelta operator""_min(long double n) {return TimeDelta::Minutes(n);}
[[nodiscard]] constexpr TimeDelta operator""_h(long double n) {return TimeDelta::Hours(n);}
[[nodiscard]] constexpr TimeDelta operator""_d(long double n) {return TimeDelta::Days(n);}

struct SystemTimestamp
{
	static SystemTimestamp Now()
	{
		int64 resNs = 0;
#ifdef _WIN32
		z_D::GetSystemTimeAsFileTime(reinterpret_cast<int64*>(&resNs));
		resNs = (resNs - 116444736000000000LL) * 100; //1 jan 1601 -> 1 jan 1970 and to nanoseconds
#elif defined(__APPLE__) && INTRA_SUPPORT_OLD_MACOS
		z_D::timeval time; z_D::gettimeofday(reinterpret_cast<timeval*>(&time), nullptr);
		resNs = time.tv_sec * 1000000000LL + time.tv_usec * 1000LL;
#else
		z_D::timespec time; z_D::clock_gettime(0, reinterpret_cast<timespec*>(&time)); // CLOCK_REALTIME
		resNs = time.tv_sec * 1000000000LL + time.tv_nsec;
#endif
		return {.Since1970 = {.ValueInSeconds = Fixed<int64, 1000000000>(Construct, resNs)}};
	}

	[[nodiscard]] INTRA_FORCEINLINE constexpr SystemTimestamp operator+(TimeDelta rhs) const {return {Since1970 + rhs};}
	[[nodiscard]] INTRA_FORCEINLINE constexpr SystemTimestamp operator-(TimeDelta rhs) const {return {Since1970 - rhs};}
	[[nodiscard]] INTRA_FORCEINLINE constexpr TimeDelta operator-(const SystemTimestamp& rhs) const {return {Since1970 - rhs.Since1970};}

	TimeDelta Since1970;
};

struct MonotonicTimestamp
{
	static MonotonicTimestamp Now()
	{
		int64 ns = 0;
#ifdef _WIN32
		static int64 queryPerformanceFrequency;
		static double nsPerQPCTick;
		if(!size_t(queryPerformanceFrequency))
		{
			z_D::QueryPerformanceFrequency(reinterpret_cast<_LARGE_INTEGER*>(&queryPerformanceFrequency));
			nsPerQPCTick = 1000000000.0 / queryPerformanceFrequency;
		}
		int64 current; z_D::QueryPerformanceCounter(reinterpret_cast<_LARGE_INTEGER*>(&current));
		ns = int64(current * nsPerQPCTick);
#elif defined(__APPLE__) && INTRA_SUPPORT_OLD_MACOS
		//TODO: this is not a monotonic clock, use mach_continuous_time() instead
		ns = SystemTimestamp::Now().Since1970.Raw;
#else
		timespec time;
#ifdef __linux__
		clock_gettime(7, Unsafe(&time)); // CLOCK_BOOTTIME
#else
		clock_gettime(1, Unsafe(&time)); // CLOCK_MONOTONIC
#endif
		ns = time.tv_sec * 1000000000LL + time.tv_nsec;
#endif
		return {.SinceEpoch = {.ValueInSeconds = Fixed<int64, 1000000000>(Construct, ns)}};
	}

	[[nodiscard]] INTRA_FORCEINLINE constexpr MonotonicTimestamp operator+(TimeDelta delta) const {return {SinceEpoch + delta};}
	[[nodiscard]] INTRA_FORCEINLINE constexpr MonotonicTimestamp operator-(TimeDelta delta) const {return {SinceEpoch - delta};}
	[[nodiscard]] INTRA_FORCEINLINE constexpr TimeDelta operator-(const MonotonicTimestamp& rhs) const {return {SinceEpoch - rhs.SinceEpoch};}

	TimeDelta SinceEpoch;
};

struct Timestamp
{
	[[nodiscard]] static INTRA_FORCEINLINE Timestamp Now()
	{
		return {
			.SystemTime = SystemTimestamp::Now(),
			.MonotonicTime = MonotonicTimestamp::Now()
		};
	}

	/// Use this to create a timestamp if you have just obtained the actual time from a server, GPS, or any other time source
	[[nodiscard]] static INTRA_FORCEINLINE Timestamp Now(SystemTimestamp systemTime) {return {systemTime, MonotonicTimestamp::Now()};}

	[[nodiscard]] constexpr SystemTimestamp SynchronizedSystemTime(Timestamp timeSource) const
	{
		timeSource.SystemTime.Since1970 -= timeSource.MonotonicTime.SinceEpoch - MonotonicTime.SinceEpoch;
		return timeSource.SystemTime;
	}
	[[nodiscard]] INTRA_FORCEINLINE SystemTimestamp SynchronizedSystemTime() const {return SynchronizedSystemTime(Now());}

	[[nodiscard]] INTRA_FORCEINLINE constexpr Timestamp operator+(TimeDelta delta) const
	{
		return {.SystemTime = SystemTime + delta, .MonotonicTime = MonotonicTime + delta};
	}
	[[nodiscard]] INTRA_FORCEINLINE constexpr Timestamp operator-(TimeDelta delta) const
	{
		return {.SystemTime = SystemTime - delta, .MonotonicTime = MonotonicTime - delta};
	}

	SystemTimestamp SystemTime;
	MonotonicTimestamp MonotonicTime;
};


struct DateTime
{
	uint16 Year = 0;
	uint8 Month = 0, Day = 0, Hour = 0, Minute = 0, Second = 0;
	uint8 WeekDay = 0;
	uint16 YearDay = 0;
	int8 LocalTimeOffsetInQuarterHours = 0;

	static INTRA_FORCEINLINE DateTime UtcNow() {return FromTimestamp(SystemTimestamp::Now(), 0);}

	static INTRA_FORCEINLINE constexpr bool IsLeapYear(int year) const noexcept
	{
		return year % 4 == 0 && year / 100 % 4 == 0;
	}

	static TimeDelta LocalOffset() const
	{
#ifdef _WIN32
		z_D::TIME_ZONE_INFORMATION timeZone;
		z_D::GetTimeZoneInformation(Unsafe(&timeZone));
		const auto bias = -timeZone.Bias;
#else
		const time_t epochPlus11h = 11 * 3600;
		const z_D::tm_* local = Unsafe(z_D::localtime(&epochPlus11h));
		const auto localMins = local->tm_hour * 60 + local->tm_min;
		const z_D::tm_* gmt = Unsafe(z_D::gmtime(&epochPlus11h));
		const auto gmtMins = gmt->tm_hour * 60 + gmt->tm_min;
		const auto bias = localMins - gmtMins;
#endif
		return TimeDelta::Minutes(bias);
	}

	constexpr SystemTimestamp ToTimestamp() const
	{
#if INTRA_PREFER_CRT_FUNCTIONS
		if(!IsConstantEvaluated())
		{
			z_D::tm tm = {
				.tm_sec = Second,
				.tm_min = Minute,
				.tm_hour = Hour,
				.tm_mday = Day,
				.tm_mon = Month - 1,
				.tm_year = Year - 1900,
				.tm_wday = WeekDay,
				.tm_yday = YearDay,
				.tm_isdst = 0
			};
			return {.Since1970 = {.ValueInSeconds = mktime(Unsafe(&tm))}};
		}
#endif
		const auto leapYearsBefore = [](uint32 y) {y += 999999999; return (y / 4) - (y / 100) + (y / 400);};
		const int numLeapYearsSince1970 = leapYearsBefore(Year) - leapYearsBefore(1971);
		const int32 daysSinceJan1ThisYear = Day - 1 + daysSinceJanNotLeap[Month - 1] + (IsLeapYear(Year) && Month >= 3);
		const int32 numDaysSince1970 = daysSinceJan1ThisYear + (Year - 1970) * 365 + numLeapYearsSince1970;
		int64 seconds = Second + Minute * 60 + (4 * Hour - LocalTimeOffsetInQuarterHours) * 900;
		seconds += numDaysSince1970 * 86400LL;
		return Timestamp{seconds * 1000000000LL};
	}

	static INTRA_NOINLINE INTRA_COLD constexpr DateTime FromTimestamp(SystemTimestamp timestamp, int localTimeOffsetInQuarterHours)
	{
#if INTRA_PREFER_CRT_FUNCTIONS
		if(!IsConstantEvaluated())
		{
			const auto time = time_t(Since1970.ValueInSeconds);
			const z_D::tm* tm = Unsafe(z_D::localtime(&time));
			return {
				.Year = uint16(1900 + tm->tm_year),
				.Month = uint8(1 + tm->tm_mon),
				.Day = uint8(tm->tm_mday),
				.Hour = uint8(tm->tm_hour),
				.Minute = uint8(tm->tm_min),
				.Second = uint8(tm->tm_sec),
				.WeekDay = uint8(tm->tm_wday),
				.YearDay = uint16(tm->tm_yday),
				.LocalTimeOffsetInQuarterHours = int8(localTimeOffsetInQuarterHours)
			};
		}
#endif
		const int64 t = int64(timestamp.Since1970.ValueInSeconds) + localTimeOffsetInQuarterHours * 900;
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
			const auto div = [](int a, int b) {return a/b - (a % b < 0);};
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
} INTRA_END
