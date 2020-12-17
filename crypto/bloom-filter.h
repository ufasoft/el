/*######   Copyright (c) 2014-2015 Ufasoft  http://ufasoft.com  mailto:support@ufasoft.com,  Sergey Pavlov  mailto:dev@ufasoft.com ####
#                                                                                                                                     #
# 		See LICENSE for licensing information                                                                                         #
#####################################################################################################################################*/

#pragma once

#include EXT_HEADER_DYNAMIC_BITSET

namespace Ext { namespace Crypto {
using namespace std;

class BloomFilter {
public:
 	dynamic_bitset<byte> Bitset;
	int HashNum;

	BloomFilter()
		:	HashNum(1)
	{}

	virtual ~BloomFilter() {}

	bool Contains(const ConstBuf& key) const;
	void Insert(const ConstBuf& key);
protected:
	virtual size_t Hash(const ConstBuf& cbuf, int n) const =0;
};



}} // Ext::Crypto::

