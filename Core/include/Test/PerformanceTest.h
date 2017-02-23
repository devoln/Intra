#pragma once

#include "Range/Generators/ArrayRange.h"
#include "IO/LogSystem.h"
#include "Range/Generators/StringView.h"
#include "Math/Random.h"
#include "Algo/Hash/Murmur.h"
#include "Container/ForwardDecls.h"

namespace Intra {

INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS

void PrintPerformanceResults(IO::Logger& logger, StringView testName,
	ArrayRange<const StringView> comparedTypes, ArrayRange<const double> stdTimes, ArrayRange<const double> times);

struct TestGroup
{
	bool yes;
	IO::Logger& logger;

	operator bool() const {return yes;}
	TestGroup(IO::Logger& Log, StringView category);
	~TestGroup();

	static int YesForNestingLevel;
private:
	static int nestingLevel;
	TestGroup& operator=(const TestGroup&) = delete;
};


template<size_t N> struct Big
{
	char c[N];
	forceinline bool operator==(const Big& rhs) const {return C::memcmp(c, rhs.c, N)==0;}
	forceinline bool operator!=(const Big& rhs) const {return !operator==(rhs);}
	forceinline bool operator<(const Big& rhs) const {return C::memcmp(c, rhs.c, N)<0;}
	Big& operator+=(const Big& rhs) {c[0]+=rhs.c[0]; return *this;}

	uint ToHash() const
	{return Algo::Hash::Murmur3_32(StringView(c, N), 0);}
};


template<typename T> forceinline Meta::EnableIf<
	Meta::IsIntegralType<T>::_
> GenerateRandomValue(T& dst)
{dst = Math::Random<T>::Global();}

template<typename T> Meta::EnableIf<
	Meta::IsFloatType<T>::_
> GenerateRandomValue(T& dst)
{dst = Math::Random<T>::Global.SignedNext()*1000;}

template<size_t N> void GenerateRandomValue(Big<N>& dst)
{
	static_assert(N % sizeof(size_t)==0, "Big must have size which is multiple of uint!");
	size_t* ptr = reinterpret_cast<size_t*>(dst.c);
	for(size_t i=0; i<N/sizeof(size_t); i++) GenerateRandomValue(*ptr++);
}

template<typename Char> void GenerateRandomValue(GenericString<Char>& dst)
{
	const size_t size = Math::Random<uint>::Global(5, 30);
	dst.SetLengthUninitialized(size);
	for(size_t i=0; i<size; i++)
		dst[i] = char('A'+Math::Random<uint>::Global(26));
}

template<typename T> ArrayRange<const T> GetRandomValueArray(size_t size)
{
	static Array<T> arr;
	size_t oldSize = arr.Count();
	if(oldSize<size) arr.SetCount(size);
	for(size_t i=oldSize; i<size; i++) GenerateRandomValue(arr[i]);
	return arr(0, size);
}

template<typename MAP> void PopulateMapRandom(MAP& map, size_t count)
{
	auto keys = GetRandomValueArray<typename MAP::key_type>(count);
	auto values = GetRandomValueArray<typename MAP::mapped_type>(count);
	for(size_t i=0; i<count; i++)
		map.insert({keys[i], values[i]});
}

template<typename String> String GenerateRandomString(size_t len)
{
	Math::Random<int> rand(uint(len)^Math::Random<uint>::Global());
	String result;
	result.resize(len);
	for(char& c: result)
	{
		int v = rand(62);
		if(v<10) c = char('0'+v);
		else if(v<36) c = char('A'+v-10);
		else c = char('a'+v-36);
	}
	return result;
}

INTRA_WARNING_POP

}

