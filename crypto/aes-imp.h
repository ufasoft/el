/*######   Copyright (c) 2013-2018 Ufasoft  http://ufasoft.com  mailto:support@ufasoft.com,  Sergey Pavlov  mailto:dev@ufasoft.com ####
#                                                                                                                                     #
# 		See LICENSE for licensing information                                                                                         #
#####################################################################################################################################*/

#pragma once

extern "C" {

extern const uint8_t g_aesSubByte[256];
extern const uint8_t *g_aesInvSubByte,
	*g_aesPowers;
extern const uint16_t *g_aesLogs;
extern const uint8_t *g_aesMul;


} // "C"

namespace Ext { namespace Crypto {

void InitAesTables();

static inline uint8_t Mul(uint8_t a, uint8_t b) {
	return g_aesMul[((int)a << 8) | b];
}



}} // Ext::Crypto
