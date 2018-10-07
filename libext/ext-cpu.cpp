/*######   Copyright (c)      2018 Ufasoft  http://ufasoft.com  mailto:support@ufasoft.com,  Sergey Pavlov  mailto:dev@ufasoft.com ####
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


#endif // UCFG_CPU_X86_X64

} // Ext::
