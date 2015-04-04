/*######   Copyright (c) 2013-2015 Ufasoft  http://ufasoft.com  mailto:support@ufasoft.com,  Sergey Pavlov  mailto:dev@ufasoft.com ####
#                                                                                                                                     #
# 		See LICENSE for licensing information                                                                                         #
#####################################################################################################################################*/

#include <el/ext.h>

#include "prime.h"

#include EXT_HEADER_DYNAMIC_BITSET

namespace Ext { namespace Num {

class PrimeNumbers : public vector<uint32_t> {
	typedef dynamic_bitset<> base;
public:
	PrimeNumbers() {
		ASSERT(MAX_PRIME_FACTOR*MAX_PRIME_FACTOR == PRIME_LIMIT);

		size_t lim = PRIME_LIMIT/2;
		dynamic_bitset<> vComposite(lim );
		for (uint32_t n=3; n<MAX_PRIME_FACTOR; n+=2) {
			if (!vComposite[n/2]) {
				for (uint32_t m=n*n/2; m<lim; m+=n)
					vComposite.set(m);
			}
		}
		push_back(2);
		for (uint32_t p=3; p<PRIME_LIMIT; p+=2)
			if (!vComposite[p/2])
				push_back(p);
	}
};

static InterlockedSingleton<PrimeNumbers> s_primeNumbers;

const vector<uint32_t>& PrimeTable() {
	return *s_primeNumbers;
}

BigInteger Primorial(uint32_t n) {
	const vector<uint32_t>& primes = PrimeTable();
	BigInteger r = 1;
	EXT_FOR (uint32_t p, primes) {
		if (p > n)
			break;
		r *= BigInteger(p);
	}
	return r;
}

}} // Ext::Num::

