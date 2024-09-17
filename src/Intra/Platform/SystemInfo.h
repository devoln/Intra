#pragma once

#include <Intra/Core.h>

// The goal of Intra is to have its own implementation for detecting the most useful features on the most common platforms (Windows/Linux, x86/ARM):
// 1. CPU logical/physical core counts for efficient thread pools (OS-specific code for Windows/Linux, for others - cpuid on x86).
// 2. Detection of CPU instruction sets that Intra may consider to use (SSE-AVX2, AES, NEON).
// 3. Cache sizes (at least on x86). Particularly, L1 data cache size is useful to estimate the best temporary buffer size.
// 4. Total/used/available system memory (not implemented yet).
// 5. Total/used/available disk space (not implemented yet).
// For better support on less popular platforms add this library as a backend: https://github.com/pytorch/cpuinfo
#if defined(__i386__) || defined(__amd64__)
#include <Intra/Platform/CpuidX86.h>
using Cpuid = CpuidX86;
#define INTRA_CPUID_SUPPORTED 1
/*#elif defined(__aarch64__)
#include <Intra/Platform/CpuidArm.h>
using Cpuid = CpuidArm;
#define INTRA_CPUID_SUPPORTED 1*/
#else
struct Cpuid {
	static INTRA_FORCEINLINE const Cpuid& Get(int cpuIndex = -1)
	{
		cpuIndex = 0; // multiple socket systems are not currently implemented
		static Cpuid cpus[1];
		return cpus[cpuIndex];
	}
};
#define INTRA_CPUID_SUPPORTED 0
#endif

struct TSystemInfo
{
	struct TCpu
	{
		struct TCluster
		{
			struct TCache
			{
				struct Level
				{
					int Size;
					int NumSets;
					int NumWays: 8;
					int NumPartitions: 8;
					int CacheLineSize: 16;
				} L1D{}, L1I{}, L2{}, L3{};

				static const TCache& Get(int clusterIndex = -1, int cpuIndex = -1)
				{
					static TCache caches[1][1];
					clusterIndex = 0;
					cpuIndex = 0;
					auto& res = caches[clusterIndex][cpuIndex];
					res.init(clusterIndex, cpuIndex);
					return res;
				}

				[[nodiscard]] operator bool() const {return L2.Size != 0;}
			private:
				void init(int clusterIndex = -1, int cpuIndex = -1) {return initFromCpuid(clusterIndex, cpuIndex);}
				friend TCluster;

				INTRA_NOINLINE void initFromCpuid(int clusterIndex = -1, int cpuIndex = -1);
			} Cache;

			struct TCores
			{
				uint16 NumLogicalCores = 0;
				uint16 NumPhysicalCores = 0;
				uint8 NumThreadsPerCore = 0;
				bool PowerEfficient = false;

				static const TCores& Get(int clusterIndex = -1, int cpuIndex = -1)
				{
					static TCores cores[1][1];
					clusterIndex = 0;
					cpuIndex = 0;
					auto& res = cores[clusterIndex][cpuIndex];
					res.init(clusterIndex, cpuIndex);
					return res;
				}
			private:
				void init(int clusterIndex = -1, int cpuIndex = -1) {initFromCpuid(clusterIndex, cpuIndex);}
				friend TCluster;

				// independent of OS way but may give incorrect results (doesn't work with hybrid CPUs, probably don't work with some old CPUs and probably has bugs) 
				INTRA_NOINLINE void initFromCpuid(int clusterIndex = -1, int cpuIndex = -1);
			} Cores;

			[[nodiscard]] operator bool() const {return bool(Cache);}

		private:
			INTRA_NOINLINE void init(int clusterIndex = -1, int cpuIndex = -1)
			{
				Cache.init(clusterIndex, cpuIndex);
				Cores.init(clusterIndex, cpuIndex);
			}
			friend TCpu;
		} Clusters[4];

		bool IsIntel: 1 = false;
		bool IsAMD: 1 = false;
		bool IsVIA: 1 = false;

		struct TInstructionSets
		{
			// unlike Cpuid these are true only when these instructions can actually be used: not only supported by CPU but also supported by the OS
			bool SSE: 1 = false;
			bool SSE2: 1 = false;
			bool SSE3: 1 = false;
			bool SSSE3: 1 = false;
			bool SSE41: 1 = false;
			bool SSE42: 1 = false;
			bool AVX: 1 = false;
			bool AVX2: 1 = false;
			bool FMA: 1 = false;

			bool NEON: 1 = false;

		private:
			void init(int cpuIndex = -1);
			friend TCpu;
		} InstructionSets;

		static const TCpu& Get(int cpuIndex = -1)
		{
			static TCpu cpus[1];
			if(cpuIndex < 0) cpuIndex = 0;
			auto& res = cpus[cpuIndex];
			res.init(cpuIndex);
			return res;
		}

		[[nodiscard]] operator bool() const {return bool(Clusters[0]);}

	private:
	#if defined(__i386__) || defined(__amd64__)
		static bool checkIs(const char name[12], const Cpuid& cpuid)
		{
			return __builtin_memcmp(cpuid.Leaf0.ManufacturerID, name, sizeof(cpuid.Leaf0.ManufacturerID)) == 0;
		};
	#endif

		INTRA_NOINLINE void init(int cpuIndex = -1)
		{
			for(int i = 0; i < sizeof(Clusters) / sizeof(Clusters[0]); i++)
				Clusters[i].init(i, cpuIndex);
			InstructionSets.init(cpuIndex);
		}
		friend TSystemInfo;
	} Cpus[4];

	static const TSystemInfo& Get()
	{
		static TSystemInfo res;
		if(!res) res.init();
		return res;
	}
	operator bool() const {return bool(Cpus[0]);}

private:
	INTRA_NOINLINE void init()
	{
		for(int i = 0; i < sizeof(Cpus) / sizeof(Cpus[0]); i++)
			Cpus[i].init(i);
		// TODO: memory, OS info, etc.
	}
};
