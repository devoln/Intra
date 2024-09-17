#pragma once

#include <Intra/Core.h>

namespace Intra { INTRA_BEGIN
#if defined(__i386__) || defined(__amd64__)
namespace z_D {
#ifdef _MSC_VER
extern "C" void __cpuidex(int[4], int, int);
extern "C" uint64 _xgetbv(unsigned index);
#elif defined(__GNUC__)
inline void __cpuidex(int* cpuinfo, int eax, int ecx) { __asm__ __volatile__(
	"cpuid\n"
	:"=a" (cpuinfo[0]), "=b" (cpuinfo[1]), "=c" (cpuinfo[2]), "=d" (cpuinfo[3])
	:"0" (eax), "1" (ecx)
);}
inline uint64 _xgetbv(uint32 index)
{
	unsigned eax, edx;
	__asm__ __volatile__(
		"xgetbv"
		: "=a" (eax), "=d"(edx)
		: "c" (index)
	);
	return (uint64(edx) << 32) | eax;
}
#endif
}

struct CpuidX86
{
	enum class CacheType: uint32 {Null, Data, Code, Unified};
	struct CacheLevelInfo
	{
		CacheType Type: 5;
		uint32 Level: 3;
		uint32 SelfInitializing: 1;
		uint32 FullyAssociative: 1;
		uint32: 4;
		uint32 NumThreadsPerCacheMinus1: 12;
		uint32 CoresPerPackageMinus1: 6;

		uint32 SystemCoherencyLineSizeMinus1: 12;
		uint32 PhysicalLinePartitionsMinus1: 10;
		uint32 NumWaysOfAssociativityMinus1: 10;

		uint32 NumSetsMinus1;

		bool WriteBackInvalidate: 1;
		bool InclusiveOfLowerLevels: 1;
		bool ComplexIndexing: 1;
		bool: 5, : 8, : 8, : 8;


		uint32 CacheLineSize() const {return SystemCoherencyLineSizeMinus1 + 1;}
		uint32 NumSets() const {return NumSetsMinus1 + 1;}
		uint32 NumWaysOfAssociativity() const {return NumWaysOfAssociativityMinus1 + 1;}
		uint32 NumPartitions() const {return PhysicalLinePartitionsMinus1 + 1;}
		uint32 SizeInBytes() const {return CacheLineSize() * NumPartitions() * NumSets() * NumWaysOfAssociativity();}
	};

	struct IntelTopologyLevel
	{
		enum class LevelType: uint8 {Null, ThreadsPerCore, Threads};
		uint32 X2ApicIdBitShift: 5;
		uint32: 21;
		uint32 PackageReservedApicMaskBits: 6;

		uint16 NumEnabledLogicalProcessors;
		uint16: 16;

		uint8 LevelNumber: 8;
		LevelType Type: 8;
	uint8: 8, : 8;

		int LogicalCpuX2ApicID; // ID of logical CPU that was running the initialization of this structure
	};

	static bool IsRunningOnEfficiencyCore()
	{
		int res[4]; z_D::__cpuidex(res, 0x1A, 0);
		return (res[0] >> 8) == 0x20; // P-core is 0x40
	}

	struct TLeaf0
	{
		int MaxLeafIndex;
		char ManufacturerID[12];
	};
	struct TLeaf1
	{
		// eax
		uint32 SteppingID: 4;
		uint32 Model: 4;
		uint32 FamilyID: 4;
		uint32 ProcessorType: 2;
		uint32: 2;
		uint32 ExtendedModelID: 4;
		uint32 ExtendedFamilyID: 8;
		uint32: 4;

		// ebx
		uint8 BrandIndex;
		uint8 CacheLineSizeDiv8; // multiply by 8 to get cache line size; CLFSH must be set
		uint8 MaxAddressableIDsForLogicalProcessors; // HTT must be set
		uint8 LocalApicID;

		// ecx
		bool SSE3: 1;
		bool PCLMULQDQ: 1;
		bool DTES64: 1;
		bool MONITOR: 1;
		bool DSCPL: 1;
		bool VMX: 1;
		bool SMX: 1;
		bool EST: 1;
		bool TM2: 1;
		bool SSSE3: 1;
		bool CNXT_ID: 1;
		bool SDBG: 1;
		bool FMA: 1;
		bool CX16: 1; // CMPXCHG16B
		bool XTPR: 1;
		bool PDCM: 1;
		bool: 1;
		bool PSID: 1;
		bool DCA: 1;
		bool SSE41: 1;
		bool SSE42: 1;
		bool X2APIC: 1;
		bool MOVBE: 1;
		bool POPCNT: 1;
		bool TSC_DEADLINE: 1;
		bool AES_NI: 1;
		bool XSAVE: 1;
		bool OSXSAVE: 1;
		bool AVX: 1; // requires OSXSAVE and OS support, must be checked together
		bool F16C: 1;
		bool RDRAND: 1;
		bool Hypervisor: 1;

		// edx
		bool FPU: 1;
		bool VME: 1;
		bool DE: 1;
		bool PSE: 1;
		bool TSC: 1;
		bool MSR: 1;
		bool PAE: 1;
		bool MCE: 1;
		bool CX8: 1;
		bool APIC: 1;
		bool: 1;
		bool SEP: 1;
		bool MTRR: 1;
		bool PGE: 1;
		bool MCA: 1;
		bool CMOV: 1;
		bool PAT: 1;
		bool PSE36: 1;
		bool PSN: 1; // Processor Serial Number supported and enabled
		bool CLFSH: 1;
		bool: 1;
		bool DS: 1;
		bool ACPI: 1;
		bool MMX: 1;
		bool FXSR: 1;
		bool SSE: 1;
		bool SSE2: 1;
		bool SS: 1;
		bool HTT: 1;
		bool TM: 1;
		bool IA64: 1;
		bool PBE: 1;
	};
	struct TLeaf2
	{
		// a list of descriptors indicating cache and TLB capabilities
		uint8 CacheAndTLBDescriptors[16];
	};
	struct TLeaf3
	{
		// not implemented in modern processors
		int ProcessorSerialNumber[4];
	};
	struct TLeaf4
	{
		CacheLevelInfo IntelCacheLevel0;
	};
	struct TLeaf5
	{
		int: 32;
		int: 32;
		int: 32;
		int: 32;
	};
	struct TLeaf6
	{
		// eax
		bool DigitalThermalSensor: 1;
		bool TurboBoost: 1;
		bool AlwaysRunningAPICTimer: 1;
		bool: 1;
		bool PowerLimitNotification: 1;
		bool ExtendedClockModulationDuty: 1;
		bool PackageThermalManagement: 1;
		bool HardwareControlledPerformanceStates: 1;
		bool HWPNotification: 1;
		bool HWPActivityWindow: 1;
		bool HWPEnergyPerformancePreferenceControl: 1;
		bool HWPPackageLevelControl: 1;
		bool: 1;
		bool HardwareDutyCycling: 1;
		bool TurboBoostMax3: 1;
		bool InterruptsUponChangesToHWPCaps: 1;
		bool HWPPECIOverrideSupported: 1;
		bool FlexibleHWP: 1;
		bool FastAccessModeMSRSupported: 1;
		bool HardwareFeedback: 1;
		bool IgnoredIdleHWPRequest: 1;
		bool: 1;
		bool HwpCtlSupported: 1;
		bool IntelThreadDirector: 1;
		bool ThermInterruptSupported: 1;
		bool: 7;

		// ebx
		uint32 NumInterruptThresholdsInDigitalThermalSensor: 4;
		uint32: 28;

		// ecx
		bool EffectiveFrequencyInterfaceSupported: 1;
		bool ACNT2: 1;
		bool: 1;
		bool PerformanceEnergyBias: 1;
		bool: 4;
		uint8 NumSupportedIntelThreadDirectors;
		uint16: 16;

		// edx
		bool PerformanceCapabilityReportingSupported: 1;
		bool EfficiencyCapabilityReportingSupported: 1;
		bool: 6;
		uint8 HardwareFeedbackInterfaceStructSize: 4; //(in units of 4 Kbytes) minus 1
		uint8: 4;
		uint16 ThisLogicalProcessorRowIndex;
	};
	struct TLeaf7
	{
		int MaxLeaf7SubLeafIndex; // eax

		// ebx
		bool FSGSBASE: 1;
		bool TscAdjust: 1;
		bool SoftwareGuardExtensions: 1;
		bool BMI1: 1;
		bool HLE: 1; // only on Intel CPUs, need to check manufacturer
		bool AVX2: 1;
		bool FdpExceptionOnly: 1;
		bool SupervisorModeExecutionPrevention: 1;
		bool BMI2: 1;
		bool ERMS: 1;
		bool INVPCID: 1;
		bool RTM: 1; // only on Intel CPUs, need to check manufacturer
		bool RdtmOrPqm: 1;
		bool X87FpuCSAndDSDeprecated: 1;
		bool MemoryProtectionExtensions: 1;
		bool RdtaOrPqe: 1;
		bool AVX512F: 1;
		bool AVX512DQ: 1;
		bool RDSEED: 1;
		bool ADX: 1;
		bool SupervisorModeAccessPrevention: 1;
		bool AVX512IFMA: 1;
		bool PCOMMIT: 1; // deprecated
		bool CLFLUSHOPT: 1;
		bool CLWB: 1; // Cache Line Writeback instruction
		bool IntelProcessorTrace: 1;
		bool AVX512PF: 1;
		bool AVX512ER: 1;
		bool AVX512CD: 1;
		bool SHA: 1;
		bool AVX512BW: 1;
		bool AVX512VL: 1;

		// ecx
		bool PREFETCHWT1: 1;
		bool: 7;
		bool: 8;
		bool: 8;
		bool: 8;

		// edx
		bool: 1;
		bool SgxKeys: 1;
		bool AVX512_4VNNIW: 1;
		bool AVX512_4FMAPS: 1;
		bool FastShortRepMovsb: 1;
		bool UserInterProcessorInterrupts: 1;
		bool: 2;
		bool AVX512VP2Intersect: 1;
		bool SpecialRegisterBufferDataSamplingMitigations: 1;
		bool McClear: 1;
		bool RtmAlwaysAbort: 1;
		bool: 1;
		bool TsxForceAbortAvailable: 1;
		bool Serialize: 1;
		bool Hybrid: 1;
		bool TSXLDTRK: 1;
		bool: 1;
		bool PConfig: 1;
		bool ArchitecturalLastBranchRecords: 1;
		bool CetIbt: 1;
		bool: 1;
		bool AMXBF16: 1;
		bool AVX512FP16: 1;
		bool AMXTile: 1;
		bool AMXInt8: 1;
		bool SpeculationControl: 1;
		bool SingleThreadIndirectBranchPredictor: 1;
		bool L1D_FLUSH: 1;
		bool SpeculativeSideChannelMitigationList: 1;
		bool ModelSpecificCoreCapList: 1;
		bool SpeculativeStoreBypassDisableMitigation: 1;
	};
	struct TLeaf7_1
	{
		// eax
		bool SHA512: 1;
		bool SM3: 1;
		bool SM4: 1;
		bool RAO_INT: 1;
		bool AVX_VNNI: 1;
		bool AVX512BF16: 1;
		bool LinearAddressSpaceSeparation: 1;
		bool CMPccXADD: 1;
		bool ArchPerfMonitoringExtLeaf: 1;
		bool: 1;
		bool Fast0LengthRepMovsb: 1;
		bool FastShortRepStosb: 1;
		bool FastShortRepCmpsbScasb: 1;
		bool: 3;
		bool: 1;
		bool FlexibleReturnAndEventDelivery: 1;
		bool LKGS: 1;
		bool WRMSRNS: 1;
		bool: 1;
		bool AMXFP16: 1;
		bool HistoryReset: 1;
		bool AVX_IFMA: 1;
		bool: 2;
		bool LinearAddressMasking: 1;
		bool MSRLIST: 1;
		bool: 4;

		// ebx
		bool IntelPPIN: 1;
		bool TotalStorageEncryption: 1;
		bool: 6;
		bool: 8;
		bool: 8;
		bool: 8;

		int: 32; // ecx

		// edx
		bool: 4;
		bool AVX_VNNI_INT8: 1;
		bool AVXNEConvert: 1;
		bool: 2;
		bool AMXComplex: 1;
		bool: 1;
		bool AVX_VNNI_INT16: 1;
		bool: 3;
		bool PREFETCHI: 1;
		bool: 1;
		bool: 1;
		bool UIRET_UIF_FromRFlags: 1;
		bool CET_SSS: 1;
		bool AVX10: 1;
		bool: 1;
		bool APX_F: 1;
		bool: 2;
		bool: 8;
	};

	struct TLeaf8
	{
		int: 32, : 32, : 32, : 32;
	};
	struct TLeaf9
	{
		int: 32, : 32, : 32, : 32;
	};
	struct TLeaf10
	{
		uint8 PerfMonVersion;
		uint8 NumPerfCounterRegs;
		uint8 PerfMonCounterBitWidth;
		uint8 PerfMonBitVectorLength;

		bool PerfMonCoreCyclesEventUnavailable: 1;
		bool PerfMonInstructionsRetiredEventUnavailable: 1;
		bool PerfMonReferenceCyclesEventUnavailable: 1;
		bool PerfMonLastLevelCacheReferencesEventUnavailable: 1;
		bool PerfMonLastLevelCacheMissesEventUnavailable: 1;
		bool PerfMonBranchInstructionsRetiredEventUnavailable: 1;
		bool PerfMonBranchMispredictsRetiredEventUnavailable: 1;
		int: 25;

		uint32 PerfMonNumFixedFunctionCounters: 5;
		uint32 PerfMonFixedFunctionCountersBitWidth: 8;
		uint32: 19;

		int: 32;
	};

	struct TLeafEx0
	{
		int MaxLeafExIndex;
		int: 32, : 32, : 32;
	};
	struct TLeafEx1
	{
		int: 32, : 32;

		bool LAHFInLongMode: 1;
		bool CmpLegacy: 1;
		bool SecureVirtualMachine: 1;
		bool ExtendedAPICSpace: 1;
		bool CR8Legacy: 1;
		bool LZCNT: 1;
		bool SSE4A: 1;
		bool MisalignSSE: 1;
		bool Amd3DNowPrefetch: 1;
		bool OSVisibleWorkaround: 1;
		bool InstructionBasedSampling: 1;
		bool AmdXOP: 1;
		bool SKINIT: 1;
		bool WatchdogTimer: 1;
		bool: 1;
		bool LightWeightProfiling: 1;
		bool AmdFMA4: 1;
		bool TranslationCacheExtension: 1;
		bool: 1;
		bool NodeIDMSR: 1;
		bool: 1;
		bool TrailingBitManipulation: 1;
		bool TopologyExtensions: 1;
		bool CorePerformanceCounterExtensions: 1;
		bool NorthbridgePerformanceCounterExtensions: 1;
		bool StreamPerfMon: 1;
		bool DataBreakpointExtensions: 1;
		bool PerformanceTimestampCounter: 1;
		bool L2IPerfCounterExtensions: 1;
		bool MONITORX: 1;
		bool AddrMaskExt: 1;
		bool: 1;

		bool: 8;
		bool: 2;
		bool SyscallK6Only: 1;
		bool Syscall: 1;
		bool: 4;
		bool: 3;
		bool AmdECC: 1; // "Athlon MP" / "Sempron" CPU brand identification
		bool NX: 1;
		bool: 1;
		bool MMXExt: 1;
		bool: 1;
		bool: 1;
		bool FXSR_OPT: 1;
		bool GigabytePages: 1;
		bool RDTSCP: 1;
		bool: 1;
		bool LongMode: 1;
		bool Ext3DNow: 1;
		bool Amd3DNow: 1;
	};
	struct TLeafEx5
	{
		struct L1TLBConfigDescriptor
		{
			uint8 NumCodeTLBEntries;
			uint8 CodeTLBAssociativity; // 255 == Full
			uint8 NumDataTLBEntries;
			uint8 DataTLBAssociativity; // 255 == Full
		};
		struct L1CacheConfigDescriptor
		{
			uint8 CacheLineSize;
			uint8 NumCacheLinesPerTag;
			uint8 CacheAssociativity; // 255 == Full
			uint8 CacheSizeKB;
		};
		L1TLBConfigDescriptor L1TLB2Or4MB;
		L1TLBConfigDescriptor L1TLB4KB;
		L1CacheConfigDescriptor L1D;
		L1CacheConfigDescriptor L1I;
	};
	struct TLeafEx6
	{
		struct L2TLBConfigDescriptor
		{
			uint32 NumCodeTLBEntries: 12;
			uint32 CodeTLBAssociativity: 4; // 0 - disabled, 0 - 1-way, 2 - 2, 3 - 3, 4 - 4, 5 - 6, 6 - 8, 8 - 16
			uint32 NumDataTLBEntries: 12;
			uint32 DataTLBAssociativity: 4;
		};
		L2TLBConfigDescriptor L2TLB2Or4MB;
		L2TLBConfigDescriptor L2TLB4KB;

		uint8 L2CacheLineSize;
		uint8 L2CacheLinesPerTag: 4;
		uint8 L2Associativity: 4;
		uint16 L2CacheSizeKB;

		uint32 L3CacheLineSize: 8;
		uint32 L3CacheLinesPerTag: 4;
		uint32 L3Associativity: 4;
		uint32: 2;
		uint32 L3CacheSizeIn512KBChunks: 14;
	};
	struct TLeafEx7
	{
		uint8 NumProcessorFeedbackPairsAvailable;
		uint8 ProcessorFeedbackCapsVersion;
		uint16 MaxWrapTime;

		bool MCAOverflowRecov: 1;
		bool SUCCOR: 1;
		bool HardwareAssertSupport: 1;
		bool ScalableMca: 1;
		bool: 4;
		bool: 8, : 8, : 8;

		uint32 CpuPwrSampleTimeRatio;

		bool TemperatureSensor: 1;
		bool FrequencyIDControl: 1;
		bool VoltageIDControl: 1;
		bool THERMTRIP: 1;
		bool HardwareThermalControl: 1;
		bool SoftwareThermalControl: 1;
		bool HundredMHzSteps: 1;
		bool HardwarePStateControl: 1;
		bool TscInvariant: 1;
		bool CorePerformanceBoost: 1;
		bool EffFreqRO: 1;
		bool ProcFeedbackInterface: 1;
		bool ProcPowerReporting: 1;
		bool ConnectedStandby: 1;
		bool RunningAveragePowerLimit: 1;
		bool FastCollaborativeProcessorPerfControl: 1;
		bool: 8, : 8;
	};

	struct TLeafEx8
	{
		uint8 NumPhysicalAddressBits;
		uint8 NumLinearAddressBits;
		uint8 GuestPhysicalAddressSize;
		uint8: 8;

		bool CLZERO: 1;
		bool RetiredInstructionCountSupported: 1;
		bool XRSTOR_FPErr: 1;
		bool INVLPGB: 1;
		bool RDPRU: 1;
		bool: 1;
		bool MemoryBandwidthEnforcement: 1;
		bool: 1;
		bool MCOMMIT: 1;
		bool WBNOINVD: 1;
		bool: 1;
		bool: 1;
		bool IndirectBranchPredictionBarrier: 1;
		bool InterruptibleWBINVD: 1;
		bool IndirectBranchRestrictedSpeculation: 1;
		bool SingleThreadIndirectBranchPredictionMode: 1;
		bool IbrsAlwaysOn: 1;
		bool StibpAlwaysOn: 1;
		bool IbrsPreferredOverSoftware: 1;
		bool IbrsSameModeProtection: 1;
		bool NoEferLmsle: 1;
		bool INVLPGBForNestedPages: 1;
		bool: 1;
		bool ProtectedProcessorInventoryNumber: 1;
		bool SpeculativeStoreBypassDisable: 1;
		bool SpeculativeStoreBypassDisableLegacy: 1;
		bool SpeculativeStoreBypassDisableNotRequired: 1;
		bool CollaborativeProcessorPerformanceControl: 1;
		bool PredictiveStoreForwardDisable: 1;
		bool NoBranchTypeConfusion: 1;
		bool IBPB_RET: 1;
		bool BranchSamplingSupport: 1;

		uint8 NumPhysicalCoresMinus1;
		uint8: 4;
		uint8 ApicIDSize: 4;
		uint8 PerfTimestampCounterSize: 2;
	uint8: 6, : 8;

		uint16 MaxPageCountForINVLPGB;
		uint16 MaxEcxValueForRDPRU;
	};

	union {
		int Leafs[11][4]{};
		struct
		{
			TLeaf0 Leaf0;
			TLeaf1 Leaf1;
			TLeaf2 Leaf2;
			TLeaf3 Leaf3;
			TLeaf4 Leaf4;
			TLeaf5 Leaf5;
			TLeaf6 Leaf6;
			TLeaf7 Leaf7;
			TLeaf8 Leaf8;
			TLeaf9 Leaf9;
			TLeaf10 Leaf10;
		};
	};
	union {
		int LeafsEx[9][4]{};
		struct
		{
			TLeafEx0 LeafEx0;
			TLeafEx1 LeafEx1;
			char ProcessorBrandString[48]; // LeafsEx[2-4]
			TLeafEx5 LeafEx5;
			TLeafEx6 LeafEx6;
			TLeafEx7 LeafEx7;
			TLeafEx8 LeafEx8;

		};
	};
	union {
		int SubLeafs4[4][4]{};
		CacheLevelInfo IntelCacheLevels[4];
	};
	union {
		int SubLeafs7[2][4]{};
		struct {
			TLeaf7 Leaf7_0;
			TLeaf7_1 Leaf7_1;
		};
	};
	union {
		int SubLeafs11[4][4]{};
		IntelTopologyLevel IntelTopologyLevels[4];
	};
	union {
		int SubLeafs27[1][4]{};

	};

	static INTRA_FORCEINLINE const CpuidX86& Get(int cpuIndex = -1)
	{
		cpuIndex = 0; // multiple socket systems are not currently implemented
		static CpuidX86 cpus[1];
		auto& res = cpus[cpuIndex];
		if(!res) res.init(cpuIndex);
		return res;
	}

private:
	CpuidX86() = default;

	INTRA_NOINLINE void init(int cpuIndex = -1)
	{
		(void)cpuIndex;
		z_D::__cpuidex(Leafs[0], 0, 0);
		std::swap(Leafs[0][2], Leafs[0][3]);
		z_D::__cpuidex(LeafsEx[0], 0x80000000, 0);
		cpuidLeafLoop(Leafs[1], 1, Leaf0.MaxLeafIndex < 10? Leaf0.MaxLeafIndex: 10, 0, false);
		cpuidLeafLoop(LeafsEx[0], 0x80000000, LeafEx0.MaxLeafExIndex <= 0x80000008? LeafEx0.MaxLeafExIndex: 0x80000008, 0, false);
		cpuidLeafLoop(SubLeafs4[0], 0, sizeof(SubLeafs4)/sizeof(SubLeafs4[0]) - 1, 4, true);
		cpuidLeafLoop(SubLeafs7[0], 0, Leaf7.MaxLeaf7SubLeafIndex <= 1? Leaf7.MaxLeaf7SubLeafIndex: 1, 7, true);
		cpuidLeafLoop(SubLeafs11[0], 0, sizeof(SubLeafs11)/sizeof(SubLeafs11[0]) - 1, 11, true);
	}

	static INTRA_NOINLINE void cpuidLeafLoop(int* dst, int i, int imax, int j, bool iterateSubLeafs)
	{
		for(; i <= imax; i++, dst += 4)
			z_D::__cpuidex(dst, iterateSubLeafs? j: i, iterateSubLeafs? i: j);
	}

	[[nodiscard]] operator bool() const {return Leaf0.MaxLeafIndex > 0;}
};
#endif
} INTRA_END
