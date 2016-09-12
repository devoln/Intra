#include "Algorithms/Hash.h"
#include "Platform/Endianess.h"

#if(INTRA_PLATFORM_ENDIANESS!=INTRA_PLATFORM_ENDIANESS_LittleEndian)
#error "Murmur2 hash does support only little endian!"
#endif

namespace Intra { namespace Hash {
#define ROTL32(x, y) ((x << y) | (x >> (32 - y)))
#define ROTL64(x, y) ((x << y) | (x >> (64 - y)))

uint Murmur3_32(StringView key, uint seed)
{
	enum: uint {c1 = 0xcc9e2d51U, c2 = 0x1b873593U, r1 = 15, r2 = 13, m = 5, n = 0xe6546b64U};

	uint hash = seed;

	const uint nblocks = uint(key.Length()/4);
	const uint* blocks = (const uint*)key.Data();
	uint k;
	for(uint i = 0; i < nblocks; i++)
	{
		k = blocks[i];
		k *= c1;
		k = ROTL32(k, r1);
		k *= c2;

		hash ^= k;
		hash = ROTL32(hash, r2) * m + n;
	}

	const byte* tail = (const byte*)(key.Data() + nblocks*4);
	uint k1 = 0;

	switch(key.Length() & 3)
	{
	case 3:
		k1 ^= tail[2] << 16;
	case 2:
		k1 ^= tail[1] << 8;
	case 1:
		k1 ^= tail[0];

		k1 *= c1;
		k1 = ROTL32(k1, r1);
		k1 *= c2;
		hash ^= k1;
	}

	hash ^= uint(key.Length());
	hash ^= (hash >> 16);
	hash *= 0x85ebca6bU;
	hash ^= (hash >> 13);
	hash *= 0xc2b2ae35U;
	hash ^= (hash >> 16);

	return hash;
}



ulong64 Murmur2_64_x64(StringView key, uint seed)
{
	const ulong64 m = 0xc6a4a7935bd1e995;
	const int r = 47;

	ulong64 h = seed ^ (key.Length() * m);

	const ulong64* data = (const ulong64*)key.Data();
	const ulong64* end = data + (key.Length()/8);

	while(data != end)
	{
		ulong64 k = *data++;

		k *= m;
		k ^= k >> r;
		k *= m;

		h ^= k;
		h *= m;
	}

	const byte* data2 = (const byte*)data;

	switch(key.Length() & 7)
	{
	case 7: h ^= ulong64(data2[6]) << 48;
	case 6: h ^= ulong64(data2[5]) << 40;
	case 5: h ^= ulong64(data2[4]) << 32;
	case 4: h ^= ulong64(data2[3]) << 24;
	case 3: h ^= ulong64(data2[2]) << 16;
	case 2: h ^= ulong64(data2[1]) << 8;
	case 1: h ^= ulong64(data2[0]);
		h *= m;
	};

	h ^= h >> r;
	h *= m;
	h ^= h >> r;

	return h;
}

ulong64 Murmur2_64_x32(StringView key, uint seed)
{
	const uint m = 0x5bd1e995;
	const int r = 24;

	uint h1 = seed ^ (uint)key.Length();
	uint h2 = 0;

	const uint* data = (const uint*)key.Data();

	while(key.End()-(char*)data>=8)
	{
		uint k1 = *data++;
		k1 *= m;
		k1 ^= k1 >> r;
		k1 *= m;
		h1 *= m;
		h1 ^= k1;

		uint k2 = *data++;
		k2 *= m;
		k2 ^= k2 >> r;
		k2 *= m;
		h2 *= m;
		h2 ^= k2;
	}

	if(key.End()-(char*)data>=4)
	{
		uint k1 = *data++;
		k1 *= m;
		k1 ^= k1 >> r;
		k1 *= m;
		h1 *= m;
		h1 ^= k1;
	}

	switch(key.End()-(char*)data)
	{
	case 3: h2 ^= ((byte*)data)[2] << 16;
	case 2: h2 ^= ((byte*)data)[1] << 8;
	case 1: h2 ^= ((byte*)data)[0];
		h2 *= m;
	};

	h1 ^= h2 >> 18;
	h1 *= m;
	h2 ^= h1 >> 22;
	h2 *= m;
	h1 ^= h2 >> 17;
	h1 *= m;
	h2 ^= h1 >> 19;
	h2 *= m;

	return (ulong64(h1) << 32)|h2;
}



forceinline uint getblock(const uint* p, intptr i)
{
	return ((uintLE*)p)[i];
}

forceinline ulong64 getblock(const ulong64* p, intptr i)
{
	return ((ulong64LE*)p)[i];
}


forceinline uint fmix(uint h)
{
	h ^= h >> 16;
	h *= 0x85ebca6b;
	h ^= h >> 13;
	h *= 0xc2b2ae35;
	h ^= h >> 16;
	return h;
}

forceinline ulong64 fmix(ulong64 k)
{
	k ^= k >> 33;
	k *= 0xff51afd7ed558ccdULL;
	k ^= k >> 33;
	k *= 0xc4ceb9fe1a85ec53ULL;
	k ^= k >> 33;
	return k;
}

hash128 Murmur3_128_x32(StringView key, uint seed)
{
	const byte* data = (const byte*)key.Data();
	const size_t nblocks = key.Length()/16;

	union
	{
		hash128 result;
		uint h[4];
	};
	h[0] = h[1] = h[2] = h[3] = seed;
	uint& h1 = h[0];
	uint& h2 = h[1];
	uint& h3 = h[2];
	uint& h4 = h[3];

	enum: uint {c1 = 0x239b961bU, c2 = 0xab0e9789U, c3 = 0x38b34ae5U, c4 = 0xa1e38b93U};

	const uint* blocks = (const uint*)(data + nblocks*16);

	for(intptr i=-(intptr)nblocks; i!=0; i++)
	{
		uint k1 = getblock(blocks, i*4);
		uint k2 = getblock(blocks, i*4+1);
		uint k3 = getblock(blocks, i*4+2);
		uint k4 = getblock(blocks, i*4+3);

		k1 *= c1;
		k1 = ROTL32(k1, 15);
		k1 *= c2;
		h1 ^= k1;

		h1 = ROTL32(h1, 19);
		h1 += h2;
		h1 = h1*5+0x561ccd1b;

		k2 *= c2;
		k2 = ROTL32(k2, 16);
		k2 *= c3; h2 ^= k2;

		h2 = ROTL32(h2, 17);
		h2 += h3;
		h2 = h2*5+0x0bcaa747U;

		k3 *= c3;
		k3 = ROTL32(k3, 17);
		k3 *= c4;
		h3 ^= k3;

		h3 = ROTL32(h3, 15);
		h3 += h4;
		h3 = h3*5+0x96cd1c35U;

		k4 *= c4;
		k4 = ROTL32(k4, 18);
		k4 *= c1;
		h4 ^= k4;

		h4 = ROTL32(h4, 13);
		h4 += h1;
		h4 = h4*5+0x32ac3b17;
	}


	const byte* tail = (const byte*)(data + nblocks*16);

	uint k1=0, k2=0, k3=0, k4=0;
	switch(key.Length() & 15)
	{
	case 15: k4 ^= tail[14] << 16;
	case 14: k4 ^= tail[13] << 8;
	case 13: k4 ^= tail[12];
		k4 *= c4;
		k4  = ROTL32(k4, 18);
		k4 *= c1;
		h4 ^= k4;

	case 12: k3 ^= tail[11] << 24;
	case 11: k3 ^= tail[10] << 16;
	case 10: k3 ^= tail[9] << 8;
	case 9: k3 ^= tail[8];
		k3 *= c3;
		k3  = ROTL32(k3, 17);
		k3 *= c4;
		h3 ^= k3;

	case 8: k2 ^= tail[7] << 24;
	case 7: k2 ^= tail[6] << 16;
	case 6: k2 ^= tail[5] << 8;
	case 5: k2 ^= tail[4];
		k2 *= c2;
		k2  = ROTL32(k2, 16);
		k2 *= c3;
		h2 ^= k2;

	case 4: k1 ^= tail[3] << 24;
	case 3: k1 ^= tail[2] << 16;
	case 2: k1 ^= tail[1] << 8;
	case 1: k1 ^= tail[0];
		k1 *= c1;
		k1  = ROTL32(k1, 15);
		k1 *= c2;
		h1 ^= k1;
	};


	h1 ^= (uint)key.Length();
	h2 ^= (uint)key.Length();
	h3 ^= (uint)key.Length();
	h4 ^= (uint)key.Length();

	h1 += h2;
	h1 += h3;
	h1 += h4;
	h2 += h1;
	h3 += h1;
	h4 += h1;

	h1 = fmix(h1);
	h2 = fmix(h2);
	h3 = fmix(h3);
	h4 = fmix(h4);

	h1 += h2;
	h1 += h3;
	h1 += h4;

	h2 += h1;
	h3 += h1;
	h4 += h1;

	return result;
}

//-----------------------------------------------------------------------------

hash128 Murmur3_128_x64(StringView key, uint seed)
{
	const byte* data = (const byte*)key.Data();
	const size_t nblocks = key.Length()/16;

	ulong64 h1 = seed, h2 = seed;

	const ulong64 c1 = 0x87c37b91114253d5ULL, c2 = 0x4cf5ad432745937fULL;


	const ulong64* blocks = (const ulong64*)data;

	for(intptr i=0; i<intptr(nblocks); i++)
	{
		ulong64 k1 = getblock(blocks, i*2);
		ulong64 k2 = getblock(blocks, i*2+1);

		k1 *= c1;
		k1 = ROTL64(k1, 31);
		k1 *= c2;
		h1 ^= k1;

		h1 = ROTL64(h1, 27);
		h1 += h2;
		h1 = h1*5+0x52dce729U;

		k2 *= c2;
		k2  = ROTL64(k2, 33);
		k2 *= c1;
		h2 ^= k2;

		h2 = ROTL64(h2, 31);
		h2 += h1;
		h2 = h2*5+0x38495ab5;
	}


	const byte* tail = (const byte*)(data + nblocks*16);

	ulong64 k1=0, k2=0;

	switch(key.Length() & 15)
	{
	case 15: k2 ^= ulong64(tail[14]) << 48;
	case 14: k2 ^= ulong64(tail[13]) << 40;
	case 13: k2 ^= ulong64(tail[12]) << 32;
	case 12: k2 ^= ulong64(tail[11]) << 24;
	case 11: k2 ^= ulong64(tail[10]) << 16;
	case 10: k2 ^= ulong64(tail[9]) << 8;
	case  9: k2 ^= ulong64(tail[8]);
		k2 *= c2;
		k2  = ROTL64(k2, 33);
		k2 *= c1;
		h2 ^= k2;

	case  8: k1 ^= ulong64(tail[7]) << 56;
	case  7: k1 ^= ulong64(tail[6]) << 48;
	case  6: k1 ^= ulong64(tail[5]) << 40;
	case  5: k1 ^= ulong64(tail[4]) << 32;
	case  4: k1 ^= ulong64(tail[3]) << 24;
	case  3: k1 ^= ulong64(tail[2]) << 16;
	case  2: k1 ^= ulong64(tail[1]) << 8;
	case  1: k1 ^= ulong64(tail[0]);
		k1 *= c1;
		k1  = ROTL64(k1, 31);
		k1 *= c2;
		h1 ^= k1;
	};

	//----------
	// finalization

	h1 ^= key.Length();
	h2 ^= key.Length();

	h1 += h2;
	h2 += h1;

	h1 = fmix(h1);
	h2 = fmix(h2);

	h1 += h2;
	h2 += h1;

	return {h1, h2};
}

}}
