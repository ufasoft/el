/*######   Copyright (c) 2013-2015 Ufasoft  http://ufasoft.com  mailto:support@ufasoft.com,  Sergey Pavlov  mailto:dev@ufasoft.com ####
#                                                                                                                                     #
# 		See LICENSE for licensing information                                                                                         #
#####################################################################################################################################*/

#pragma once

#include <el/bignum.h>

namespace Ext { namespace Num {

const size_t MAX_PRIME_FACTOR = 5000,
	PRIME_LIMIT = MAX_PRIME_FACTOR * MAX_PRIME_FACTOR;

const vector<uint32_t>& PrimeTable();

BigInteger Primorial(uint32_t n);

}} // Ext::Num::



