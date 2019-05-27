/*######   Copyright (c) 2018-2019 Ufasoft  http://ufasoft.com  mailto:support@ufasoft.com,  Sergey Pavlov  mailto:dev@ufasoft.com ####
#                                                                                                                                     #
# 		See LICENSE for licensing information                                                                                         #
#####################################################################################################################################*/

#include <el/ext.h>

namespace Ext {
using namespace std;

#if UCFG_CPU_X86_X64

CpuInfo::FeatureInfo::FeatureInfo() {
	ZeroStruct(_self);
	auto maxFun = Cpuid(0).EAX;
	IdInfo1 = Cpuid(1);
	if (maxFun >= 7)
		IdInfo7 = Cpuid(7);
}

const CpuInfo::FeatureInfo& CpuInfo::get_Features() {
	static FeatureInfo s_features;
	return s_features;
}

struct CpuInfo::SFamilyModelStepping CpuInfo::get_FamilyModelStepping() {
    int v = Cpuid(1).EAX;
	SFamilyModelStepping r = { (v>>8) & 0xF, (v>>4) & 0xF, v & 0xF };
	if (r.Family == 0xF)
		r.Family += (v>>20) & 0xFF;
	if (r.Family >= 6)
		r.Model += (v>>12) & 0xF0;
	return r;
}

bool CpuInfo::get_ConstantTsc() {
	CpuidInfo info = Cpuid(0x80000000);
	if (uint32_t(info.EAX) >= 0x80000007UL && (info.EDX & 0x100))
		return true;
	if (!strcmp(get_Vendor().Name, "GenuineIntel")) {
		SFamilyModelStepping fms = get_FamilyModelStepping();
		return 0xF==fms.Family && fms.Model==3 ||							// P4, Xeon have constant TSC
				6==fms.Family && fms.Model>=0xE && fms.Model<0x1A;			// Core [2][Duo] - constant TSC
																			// Atoms - variable TSC
	}
	return false;
}


#endif // UCFG_CPU_X86_X64

} // Ext::
