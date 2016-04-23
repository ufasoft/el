/*######   Copyright (c) 2013-2015 Ufasoft  http://ufasoft.com  mailto:support@ufasoft.com,  Sergey Pavlov  mailto:dev@ufasoft.com ####
#                                                                                                                                     #
# 		See LICENSE for licensing information                                                                                         #
#####################################################################################################################################*/

#pragma once

extern "C" {

extern const byte g_aesSubByte[256];
extern const byte *g_aesInvSubByte,
	*g_aesPowers;
extern const uint16_t *g_aesLogs;


} // "C"

namespace Ext { namespace Crypto {

void InitAesTables();

static inline byte Mul(byte a, byte b) {
	return g_aesPowers[g_aesLogs[a] + g_aesLogs[b]];
}



}} // Ext::Crypto
