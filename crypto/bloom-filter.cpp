/*######   Copyright (c) 2014-2015 Ufasoft  http://ufasoft.com  mailto:support@ufasoft.com,  Sergey Pavlov  mailto:dev@ufasoft.com ####
#                                                                                                                                     #
# 		See LICENSE for licensing information                                                                                         #
#####################################################################################################################################*/

#include <el/ext.h>

#include "bloom-filter.h"

namespace Ext { namespace Crypto {

bool BloomFilter::Contains(RCSpan key) const {
	for (int i=0; i<HashNum; ++i)
		if (!Bitset[Hash(key, i)])
			return false;
	return true;
}

void BloomFilter::Insert(RCSpan key) {
	for (int i=0; i<HashNum; ++i)
		Bitset.set(Hash(key, i));
}




}} // Ext::Crypto::


