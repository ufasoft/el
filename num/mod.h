/*######   Copyright (c) 2013-2015 Ufasoft  http://ufasoft.com  mailto:support@ufasoft.com,  Sergey Pavlov  mailto:dev@ufasoft.com ####
#                                                                                                                                     #
# 		See LICENSE for licensing information                                                                                         #
#####################################################################################################################################*/

#pragma once

// Modular arithmetic

#include <el/num/num.h>

namespace Ext { namespace Num {

class ModNum {
public:
	BigInteger m_val;
	BigInteger m_mod;

	ModNum(const BigInteger& val, const BigInteger& mod)
		:	m_val(val)
		,	m_mod(mod)
	{}
};

ModNum inverse(const ModNum& mn);
uint32_t inverse(uint32_t x, uint32_t mod);

Bn PowM(const Bn& x, const Bn& e, const Bn& mod);
ModNum pow(const ModNum& mn, const BigInteger& p);


}} // Ext::Num::

