/*######   Copyright (c) 2018-2019 Ufasoft  http://ufasoft.com  mailto:support@ufasoft.com,  Sergey Pavlov  mailto:dev@ufasoft.com ####
#                                                                                                                                     #
# 		See LICENSE for licensing information                                                                                         #
#####################################################################################################################################*/

#pragma once

namespace Ext {

struct CpuVendor {
	char Name[13];

	CpuVendor() {
		ZeroStruct(Name);
	}
};

class CpuInfo {
	typedef CpuInfo class_type;
public:
#if UCFG_CPU_X86_X64

	union CpuidInfo {
		struct {
			int EAX, EBX, ECX, EDX;
		};

		int m_ar[4];
	};

	struct FeatureInfo {
		FeatureInfo();

		union {
			CpuidInfo IdInfo1;

			struct {
				int : 32;

				int : 32;

				bool SSE3 : 1,
					PCLMULQDQ : 1,
					DTES64 : 1,
					MONITOR : 1,
					DS_CPL : 1,
					VMX : 1,
					SMX : 1,
					EST : 1,
					TM2 : 1,
					SSSE3 : 1,
					CNXT_ID: 1,
					SDBG : 1,
					FMA : 1,
					CX16 : 1,
					XPTR : 1,
					PDCM : 1,
					: 1,
					PCID : 1,
					DCA : 1,
					SSE41 : 1,
					SSE42 : 1,
					X2APIC : 1,
					MOVBE : 1,
					POPCNT : 1,
					TSC_DEADLINE : 1,
					AES : 1,
					XSAVE : 1,
					OXSAVE : 1,
					AVX : 1,
					F16C : 1,
					RDRND : 1,
					HYPERVISOR : 1;

				bool FPU : 1,
					VME : 1,
					DE : 1,
					PSE : 1,
					TSC : 1,
					MSR : 1,
					PAE : 1,
					MCE : 1,
					CX8 : 1,
					APIC : 1,
					: 1,
					SEP : 1,
					MTRR : 1,
					PGE : 1,
					MCA : 1,
					CMOV : 1,
					PAT : 1,
					PSE36 : 1,
					PSN : 1,
					CLFSH : 1,
					: 1,
					DS : 1,
					ACPI : 1,
					MMX : 1,
					FXSR : 1,
					SSE : 1,
					SSE2 : 1,
					SS : 1,
					HTT : 1,
					TM : 1,
					IA64 : 1,
					PBE;
			};
		};
		union {
			CpuidInfo IdInfo7;

			struct {
				int : 32;

				bool FSGSBASE : 1,
					: 2,
					BMI1 : 1,
					: 1,
					AVX2 : 1,
					: 2,
					BMI2 : 1,
					ERMS : 1,
					INVPCID : 1,
					: 5,
					AVX512F : 1,
					AVX512DQ : 1,
					RDSEED : 1,
					ADX : 1,
					: 1,
					AVX512IFMA : 1,
					: 4,
					AVX512PF : 1,
					AVX512ER : 1,
					AVX512CD : 1,
					SHA : 1,
					AVX512BW : 1,
					AVX512VL : 1;
				int : 32;

				int : 32;
			};
		};
	};

	struct SFamilyModelStepping {
		int Family, Model, Stepping;
	};

	static CpuidInfo Cpuid(int level) {
		CpuidInfo r;
		::Cpuid(r.m_ar, level);
		return r;
	}

	const FeatureInfo& get_Features();
	DEFPROP_GET(FeatureInfo, Features);

	CpuVendor get_Vendor() {
		int a[4];
		::Cpuid(a, 0);
		CpuVendor r;
		*(int*)r.Name = a[1];
		*(int*)(r.Name+4) = a[3];
		*(int*)(r.Name+8) = a[2];
		return r;
	}
	DEFPROP_GET(CpuVendor, Vendor);

	struct SFamilyModelStepping get_FamilyModelStepping();
	DEFPROP_GET(SFamilyModelStepping, FamilyModelStepping);

	bool get_ConstantTsc();
	DEFPROP_GET(bool, ConstantTsc);

#	if UCFG_FRAMEWORK && !defined(_CRTBLD)
	String get_Name() {
		if (uint32_t(Cpuid(0x80000000).EAX) < 0x80000004UL)
			return "Unknown";
    	int ar[13];
    	ZeroStruct(ar);
		::Cpuid(ar, 0x80000002);
		::Cpuid(ar+4, 0x80000003);
		::Cpuid(ar+8, 0x80000004);
		return String((const char*)ar).Trim();
	}
	DEFPROP_GET(String, Name);
#	endif

#endif // UCFG_CPU_X86_X64
};


} // Ext::
