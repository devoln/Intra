#pragma once

#include "Intra/Core.h"
#include "Random/FastUniform.h"
#include "IntraX/Hash/Murmur.h"
#include "IntraX/Container/ForwardDecls.h"

namespace Intra { INTRA_BEGIN
template<size_t N> struct Big
{
	char c[N];
	bool operator==(const Big& rhs) const {return z_D::memcmp(c, rhs.c, N) == 0;}
	bool operator!=(const Big& rhs) const {return !operator==(rhs);}
	bool operator<(const Big& rhs) const {return z_D::memcmp(c, rhs.c, N) < 0;}
	Big& operator+=(const Big& rhs) {c[0] += rhs.c[0]; return *this;}

	unsigned ToHash() const {return Hash::Murmur3_32(StringView(c, N), 0);}
};


template<typename T> Requires<
	CBasicIntegral<T>
> GenerateRandomValue(T& dst)
{
	auto rand = FastUniform<T>(unsigned(size_t(&dst)));
	dst = rand();
}

template<typename T> Requires<
	CBasicFloatingPoint<T>
> GenerateRandomValue(T& dst)
{
	auto rand = FastUniform<T>(unsigned(&dst));
	dst = rand()*1000;
}

template<size_t N> void GenerateRandomValue(Big<N>& dst)
{
	static_assert(N % sizeof(size_t) == 0, "Big must have size which is multiple of unsigned!");
	size_t* ptr = reinterpret_cast<size_t*>(dst.c);
	for(size_t i = 0; i < N/sizeof(size_t); i++) GenerateRandomValue(*ptr++);
}

template<typename Char> void GenerateRandomValue(GenericString<Char>& dst)
{
	auto rand = FastUniform<byte>(unsigned(size_t(&dst)));
	const size_t size = rand(5, 30);
	dst.SetLengthUninitialized(size);
	for(size_t i = 0; i < size; i++)
		dst[i] = char('A' + rand(26));
}

template<typename T> Span<const T> GetRandomValueArray(size_t size)
{
	static Array<T> arr;
	size_t oldSize = arr.Count();
	if(oldSize<size) arr.SetCount(size);
	for(size_t i = oldSize; i < size; i++) GenerateRandomValue(arr[i]);
	return arr.Take(size);
}

template<typename MAP> void PopulateMapRandom(MAP& map, size_t count)
{
	auto keys = GetRandomValueArray<typename MAP::key_type>(count);
	auto values = GetRandomValueArray<typename MAP::mapped_type>(count);
	for(size_t i = 0; i < count; i++)
		map.insert({keys[i], values[i]});
}

template<typename S> S GenerateRandomString(size_t len)
{
	FastUniform<int> rand((unsigned(len)));
	S result;
	result.resize(len);
	for(char& c: result)
	{
		const int v = rand(62);
		if(v<10) c = char('0' + v);
		else if(v<36) c = char('A' + v - 10);
		else c = char('a' + v - 36);
	}
	return result;
}
} INTRA_END
