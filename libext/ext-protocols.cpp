/*######   Copyright (c) 1997-2019 Ufasoft  http://ufasoft.com  mailto:support@ufasoft.com,  Sergey Pavlov  mailto:dev@ufasoft.com ####
#                                                                                                                                     #
# 		See LICENSE for licensing information                                                                                         #
#####################################################################################################################################*/

#include <el/ext.h>

#include <el/libext/ext-net.h>

namespace Ext {
using namespace std;

uint16_t AFXAPI CalculateWordSum(RCSpan mb, uint32_t sum, bool bComplement) {
	uint16_t *p = (uint16_t*)mb.data();
	for (size_t count = mb.size() >> 1; count--;)
		sum += *p++;
	if (mb.size() & 1)
		sum += *(uint8_t*)p;
	for (uint32_t w; w = sum >> 16;)
		sum = (sum & 0xFFFF) + w;
	return uint16_t(bComplement ? ~sum : sum);
}






} // Ext::
